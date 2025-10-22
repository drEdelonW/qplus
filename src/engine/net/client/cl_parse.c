/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_parse.c  -- parse a message received from the server

#include <string.h>
#include <stdlib.h>
#include "client.h"
#include "server.h"
#include "render.h"
#include "host.h"
#include "protocol.h"
#include "sound.h"
#include "cdaudio.h"
#include "msg.h"
#include "sys.h"
#include "console.h"
#include "common.h"
#include "sbar.h"
#include "cmd.h"
#include "cvar_q1.h"



cString svc_strings[] = {
    "svc_bad",
    "svc_nop",
    "svc_disconnect",
    "svc_updatestat",
    "svc_version",      // [long] server version
    "svc_setview",      // [short] entity number
    "svc_sound",        // <see code>
    "svc_time",         // [float] server time
    "svc_print",        // [string] null terminated string
    "svc_stufftext",    // [string] stuffed into client's console buffer
    // the string should be \n terminated
    "svc_setangle",     // [vec3] set the view angle to this absolute value

    "svc_serverinfo",   // [long] version
    // [string] signon string
    // [string]..[0]model cache [string]...[0]sounds cache
    // [string]..[0]item cache
    "svc_lightstyle",   // [byte] [string]
    "svc_updatename",   // [byte] [string]
    "svc_updatefrags",  // [byte] [short]
    "svc_clientdata",   // <shortbits + data>
    "svc_stopsound",    // <see code>
    "svc_updatecolors", // [byte] [byte]
    "svc_particle",     // [vec3] <variable>
    "svc_damage",       // [byte] impact [byte] blood [vec3] from

    "svc_spawnstatic",
    "OBSOLETE svc_spawnbinary",
    "svc_spawnbaseline",

    "svc_temp_entity",  // <variable>
    "svc_setpause",
    "svc_signonnum",
    "svc_centerprint",
    "svc_killedmonster",
    "svc_foundsecret",
    "svc_spawnstaticsound",
    "svc_intermission",
    "svc_finale",       // [string] music [string] text
    "svc_cdtrack",      // [byte] track [byte] looptrack
    "svc_sellscreen",
    "svc_cutscene"
};

//=============================================================================

/*
    ===============
    CL_EntityNum

    This error checks and tracks the total number of entities
    ===============
*/
r_Entity_p CL_EntityNum(int num) {
    if (num >= cl.num_entities) {
        if (num >= MAX_EDICTS) {
            Host_Error("CL_EntityNum: %i is an invalid number", num);
        }
        while (cl.num_entities <= num) {
            cl_entities[cl.num_entities].colormap = vid.colormap;
            cl.num_entities++;
        }
    }

    return &cl_entities[num];
}


/*
    ==================
    CL_ParseStartSoundPacket
    ==================
*/
void CL_ParseStartSoundPacket() {
    int field_mask = MSG_ReadByte();

    int volume = (field_mask & SND_VOLUME) ? MSG_ReadByte() : DEFAULT_SOUND_PACKET_VOLUME;
    float attenuation = (field_mask & SND_ATTENUATION) ? (MSG_ReadByte() / 64.0) : DEFAULT_SOUND_PACKET_ATTENUATION;

    int channel = MSG_ReadShort();
    int sound_num = MSG_ReadByte();

    int ent = channel >> 3;
    channel &= 7;

    if (ent > MAX_EDICTS)
        Host_Error("CL_ParseStartSoundPacket: ent = %i", ent);

#if 0
    vec3_t pos;
    for (int i = 0; i < VECT_DIM; i++) {
        pos[i] = MSG_ReadCoord();
    }
#else
    vec3_t pos = {
        MSG_ReadCoord(),
        MSG_ReadCoord(),
        MSG_ReadCoord()
    };
#endif
    S_StartSound(
        ent, channel,
        cl.sound_precache[sound_num], pos,
        (volume / 255.0f), attenuation
    );
}

/*
    ==================
    CL_KeepaliveMessage

    When the client is taking a long time to load stuff, send keepalive messages
    so the server doesn't disconnect.
    ==================
*/
void CL_KeepaliveMessage() {
    static float lastmsg;
    uint8_t  olddata[8192];

    if ((sv.active) || // no need if server is local
        (cls.demoplayback)) {
        return;
    }

    // read messages from server, should just be nops
    sizebuf_t old = net_message;
    memcpy(olddata, net_message.data, net_message.cursize);

    int ret;
    do {
        ret = CL_GetMessage();
        switch (ret) {
        default:    Host_Error("CL_KeepaliveMessage: CL_GetMessage failed");    break;
            // case 0:                                                                 break; // nothing waiting
        case 1:     Host_Error("CL_KeepaliveMessage: received a message");      break;
        case 2:
            if (MSG_ReadByte() != svc_nop)
                Host_Error("CL_KeepaliveMessage: datagram wasn't a nop");
            break;
        }
    } while (ret);

    net_message = old;
    memcpy(net_message.data, olddata, net_message.cursize);

    // check time
    float time = Sys_FloatTime();
    if ((time - lastmsg) < 5.0f)    return;
    lastmsg = time;

    // write out a nop
    Con_Printf("--> client to server keepalive\n");

    MSG_WriteByte(&cls.message, clc_nop);
    NET_SendMessage(cls.netcon, &cls.message);
    SZ_Clear(&cls.message);
}

/*
    ==================
    CL_ParseServerInfo
    ==================
*/
void CL_ParseServerInfo() {
    Con_DPrintf("Serverinfo packet received.\n");

    // wipe the ClientState_t struct
    CL_ClearState();

    // parse protocol version number
    int32_t ver = MSG_ReadLong();
    if (ver != PROTOCOL_VERSION) {
        Con_Printf("Server returned version %i, not %i", ver, PROTOCOL_VERSION);
        return;
    }

    // parse maxclients
    cl.maxclients = MSG_ReadByte();
    if ((cl.maxclients < 1) ||
        (cl.maxclients > MAX_SCOREBOARD)) {
        Con_Printf("Bad maxclients (%u) from server\n", cl.maxclients);
        return;
    }
    cl.scores = Hunk_AllocName(cl.maxclients * sizeof(*cl.scores), "scores");

    cl.gametype = MSG_ReadByte();   // parse gametype

    cString str = MSG_ReadString(); // parse signon message
    strncpy(cl.levelname, str, (sizeof(cl.levelname) - 1));

    // seperate the printfs so the server message can have a color
    Con_Printf("\n\n" CON_HORIZONLINE);
    Con_Printf("%c%s\n", 2, str);

    //
    // first we go through and touch all of the precache data that still
    // happens to be in the cache, so precaching something else doesn't
    // needlessly purge it
    //

    // precache models
    char model_precache[MAX_MODELS][MAX_QPATH];
    memset(cl.model_precache, 0, sizeof(cl.model_precache));
    int nummodels;
    for (nummodels = 1; ; nummodels++) {
        str = MSG_ReadString();
        if (!str[0])    break;

        if (nummodels == MAX_MODELS) {
            Con_Printf("Server sent too many model precaches\n");
            return;
        }
        strcpy(model_precache[nummodels], str);
        Mod_TouchModel(str);
    }

    // precache sounds
    char sound_precache[MAX_SOUNDS][MAX_QPATH];
    memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
    int numsounds;
    for (numsounds = 1; ; numsounds++) {
        str = MSG_ReadString();
        if (!str[0])    break;

        if (numsounds == MAX_SOUNDS) {
            Con_Printf("Server sent too many sound precaches\n");
            return;
        }
        strcpy(sound_precache[numsounds], str);
        S_TouchSound(str);
    }

    //
    // now we try to load everything else until a cache allocation fails
    //

    for (int i = 1; i < nummodels; i++) {
        cl.model_precache[i] = Mod_ForName(model_precache[i], false);
        if (cl.model_precache[i] == NULL) {
            Con_Printf("Model %s not found\n", model_precache[i]);
            return;
        }
        CL_KeepaliveMessage();
    }

    S_BeginPrecaching();
    for (int i = 1; i < numsounds; i++) {
        cl.sound_precache[i] = S_PrecacheSound(sound_precache[i]);
        CL_KeepaliveMessage();
    }
    S_EndPrecaching();


    // local state
    cl_entities[0].model = cl.worldmodel = cl.model_precache[1];

    R_NewMap();

    Hunk_Check();  // make sure nothing is hurt

    noclip_anglehack = false;  // noclip is turned off at start
}


/*
    ==================
    CL_ParseUpdate

    Parse an entity update message from the server
    If an entities model or origin changes from frame to frame, it must be
    relinked.  Other attributes can change without relinking.
    ==================
*/
int bitcounts[16];

void CL_ParseUpdate(update_bits_t bits) {
    if (cls.signon == SIGNONS - 1) { // first update is the final signon stage
        cls.signon = SIGNONS;
        CL_SignonReply();
    }

    if (bits & U_MOREBITS) {
        int ext = MSG_ReadByte();
        if (msg_badread) { Host_Error("CL_ParseUpdate: bad MOREBITS"); }
        bits |= ((uint32_t)ext) << 8;
    }

    int num = (bits & U_LONGENTITY) ? MSG_ReadShort() : MSG_ReadByte();

    r_Entity_p ent = CL_EntityNum(num);

    for (int i = 0; i < 16; i++) {
        if (bits & (1u << i)) {
            bitcounts[i]++;
        }
    }

    bool forcelink = (ent->msgtime != cl.mtime[1]); // no previous frame to lerp from

    ent->msgtime = cl.mtime[0];

    int modnum = (bits & U_MODEL) ? MSG_ReadByte() : ent->baseline.modelindex;
    if (modnum >= MAX_MODELS) { Host_Error("CL_ParseModel: bad modnum"); }

    Model_p model = cl.model_precache[modnum];
    if (model != ent->model) {
        ent->model = model;
        // automatic animation (torches, etc) can be either all together
        // or randomized
        if (model) {
            ent->syncbase = (model->synctype == ST_RAND) ? ((float)(rand() & 0x7fff) / 0x7fff) : 0.0;
        }
        else {
            forcelink = true; // hack to make null model players work
        }
#ifdef GLQUAKE
        if ((num > 0) && (num <= cl.maxclients))
            R_TranslatePlayerSkin(num - 1);
#endif
    }

    ent->frame = (bits & U_FRAME) ? MSG_ReadByte() : ent->baseline.frame;
    uint8_t i = (bits & U_COLORMAP) ? MSG_ReadByte() : ent->baseline.colormap;

    if (!i) { ent->colormap = vid.colormap; }
    else {
        if (i > cl.maxclients)  Sys_Error("i >= cl.maxclients %d > %d", i, cl.maxclients);
        ent->colormap = cl.scores[i - 1].translations;
    }

#ifdef GLQUAKE
    int skin = (bits & U_SKIN) ? MSG_ReadByte() : ent->baseline.skin;

    if (skin != ent->skinnum) {
        ent->skinnum = skin;
        if ((num > 0) && (num <= cl.maxclients))
            R_TranslatePlayerSkin(num - 1);
    }

#else
    ent->skinnum = (bits & U_SKIN) ? MSG_ReadByte() : ent->baseline.skin;
#endif
    ent->effects = (bits & U_EFFECTS) ? MSG_ReadByte() : ent->baseline.effects;

    // shift the known values for interpolation
    VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
    VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);

    ent->msg_origins[0][0] = (bits & U_ORIGIN1) ? MSG_ReadCoord() : ent->baseline.origin[0];
    ent->msg_angles[0][0] = (bits & U_ANGLE1) ? MSG_ReadAngle() : ent->baseline.angles[0];
    ent->msg_origins[0][1] = (bits & U_ORIGIN2) ? MSG_ReadCoord() : ent->baseline.origin[1];
    ent->msg_angles[0][1] = (bits & U_ANGLE2) ? MSG_ReadAngle() : ent->baseline.angles[1];
    ent->msg_origins[0][2] = (bits & U_ORIGIN3) ? MSG_ReadCoord() : ent->baseline.origin[2];
    ent->msg_angles[0][2] = (bits & U_ANGLE3) ? MSG_ReadAngle() : ent->baseline.angles[2];

    if (bits & U_NOLERP)
        ent->forcelink = true;

    if (forcelink) { // didn't have an update last message
        VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
        VectorCopy(ent->msg_origins[0], ent->origin);
        VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);
        VectorCopy(ent->msg_angles[0], ent->angles);
        ent->forcelink = true;
    }
}

/*
    ==================
    CL_ParseBaseline
    ==================
*/
void CL_ParseBaseline(r_Entity_p ent) {
    ent->baseline.modelindex = MSG_ReadByte();
    ent->baseline.frame = MSG_ReadByte();
    ent->baseline.colormap = MSG_ReadByte();
    ent->baseline.skin = MSG_ReadByte();
    for (int i = 0; i < VECT_DIM; i++) {
        ent->baseline.origin[i] = MSG_ReadCoord();
        ent->baseline.angles[i] = MSG_ReadAngle();
    }
}


/*
    ==================
    CL_ParseClientdata

    Server information pertaining to this client only
    ==================
*/
void CL_ParseClientdata(server_update_bits_t bits) {
    bool upd = false;

    cl.viewheight = (bits & SU_VIEWHEIGHT) ? MSG_ReadChar() : DEFAULT_VIEWHEIGHT;
    cl.idealpitch = (bits & SU_IDEALPITCH) ? MSG_ReadChar() : 0;

    VectorCopy(cl.mvelocity[0], cl.mvelocity[1]);
    for (int i = 0; i < VECT_DIM; i++) {
        cl.punchangle[i] = (bits & (SU_PUNCH1 << i)) ? MSG_ReadChar() : 0;
        cl.mvelocity[0][i] = (bits & (SU_VELOCITY1 << i)) ? (MSG_ReadChar() * 16) : 0;
    }
    int32_t msg;
    // [always sent]    SU_ITEMS
    if (bits & SU_ITEMS) {
        msg = MSG_ReadLong();
        if (cl.items != msg) { // set flash times
            upd = true; /* Sbar_Changed(); */
            for (int i = 0; i < 32; i++) {
                if ((msg & (1u << i)) &&
                    !(cl.items & (1u << i))
                    ) {
                    cl.item_gettime[i] = cl.time;
                }
            }
            cl.items = msg;
        }
    }

    cl.onground = (bits & SU_ONGROUND) != 0;
    cl.inwater = (bits & SU_INWATER) != 0;

    cl.stats[STAT_WEAPONFRAME] = (bits & SU_WEAPONFRAME) ? MSG_ReadByte() : 0;

    msg = (bits & SU_ARMOR) ? MSG_ReadByte() : 0;   if (cl.stats[STAT_ARMOR] != msg) { cl.stats[STAT_ARMOR] = msg; upd = true; /* Sbar_Changed(); */ }
    msg = (bits & SU_WEAPON) ? MSG_ReadByte() : 0;  if (cl.stats[STAT_WEAPON] != msg) { cl.stats[STAT_WEAPON] = msg; upd = true; /* Sbar_Changed(); */ }

    msg = MSG_ReadShort();  if (cl.stats[STAT_HEALTH] != msg) { cl.stats[STAT_HEALTH] = msg; upd = true; /* Sbar_Changed(); */ }
    msg = MSG_ReadByte();   if (cl.stats[STAT_AMMO] != msg) { cl.stats[STAT_AMMO] = msg; upd = true; /* Sbar_Changed(); */ }

    for (int i = 0; i < 4; i++) {
        int msg = MSG_ReadByte(); if (cl.stats[STAT_SHELLS + i] != msg) { cl.stats[STAT_SHELLS + i] = msg; upd = true; /* Sbar_Changed(); */ }
    }

    msg = MSG_ReadByte();
    int w = (standard_quake) ? msg : (1U << msg);
    if (cl.stats[STAT_ACTIVEWEAPON] != w) { cl.stats[STAT_ACTIVEWEAPON] = w; upd = true; /* Sbar_Changed(); */ }

    if (upd) { Sbar_Changed(); }
}

/*
    =====================
    CL_NewTranslation
    =====================
*/
void CL_NewTranslation(int32_t slot) {
    if ((slot < 0) ||
        (slot > cl.maxclients))
        Sys_Error("CL_NewTranslation: bad slot %d (max %d)", slot, cl.maxclients);

    uint8_p dest = cl.scores[slot].translations;
    uint8_p source = vid.colormap;

    memcpy(dest, source, sizeof(cl.scores[slot].translations));

    int top = (cl.scores[slot].colors & 0xF0);       // tshort
    int bottom = (cl.scores[slot].colors & 0x0F) << 4;  // pents
#ifdef GLQUAKE
    R_TranslatePlayerSkin(slot);
#endif

    for (int i = 0; i < VID_GRADES; i++, dest += 256, source += 256) {
        if (top < 128) // the artists made some backwards ranges.  sigh.
            memcpy(dest + TOP_RANGE, source + top, 16);
        else
            for (int j = 0; j < 16; j++)
                dest[TOP_RANGE + j] = source[top + 15 - j];

        if (bottom < 128)
            memcpy(dest + BOTTOM_RANGE, source + bottom, 16);
        else
            for (int j = 0; j < 16; j++)
                dest[BOTTOM_RANGE + j] = source[bottom + 15 - j];
    }
}

/*
    =====================
    CL_ParseStatic
    =====================
*/
void CL_ParseStatic() {
    int statics = cl.num_statics;
    if (statics >= MAX_STATIC_ENTITIES)
        Host_Error("Too many static entities");
    r_Entity_p ent = &cl_static_entities[statics];
    cl.num_statics++;
    CL_ParseBaseline(ent);

    // copy it to the current state
    ent->model = cl.model_precache[ent->baseline.modelindex];
    ent->frame = ent->baseline.frame;
    ent->colormap = vid.colormap;
    ent->skinnum = ent->baseline.skin;
    ent->effects = ent->baseline.effects;

    VectorCopy(ent->baseline.origin, ent->origin);
    VectorCopy(ent->baseline.angles, ent->angles);
    R_AddEfrags(ent);
}

/*
    ===================
    CL_ParseStaticSound
    ===================
*/
void CL_ParseStaticSound() {
    vec3_t org = {
        MSG_ReadCoord(),
        MSG_ReadCoord(),
        MSG_ReadCoord()
    };
    int sound_num = MSG_ReadByte();
    int vol = MSG_ReadByte();
    int atten = MSG_ReadByte();

    S_StaticSound(cl.sound_precache[sound_num], org, vol, atten);
}


#define SHOWNET(x) if (cl_shownet.value == 2) {Con_Printf("%3i:%s\n", (msg_readcount - 1), x);}

/*
    =====================
    CL_ParseServerMessage
    =====================
*/
void CL_ParseServerMessage() {
    // if recording demos, copy the message out
    if (cl_shownet.value == 1)      Con_Printf("%i ", net_message.cursize);
    else if (cl_shownet.value == 2) Con_Printf("------------------\n");

    cl.onground = false; // unless the server says otherwise
    // parse the message
    MSG_BeginReading();

    while (1) {
        if (msg_badread)    Host_Error("CL_ParseServerMessage: Bad server message");

        svc_t cmd = MSG_ReadByte();

        if (msg_badread) {
            SHOWNET("END OF MESSAGE");
            return;  // end of message
        }

        // if the high bit of the command byte is set, it is a fast update
        if (cmd & 0x80) {
            SHOWNET("fast update");
            CL_ParseUpdate(cmd & 0x7F);
            continue;
        }

        SHOWNET(svc_strings[cmd]);

        // other commands
        // int msg;
        switch (cmd) {
        default:            Host_Error("CL_ParseServerMessage: Illegible server message\n"); break;

        case svc_nop:       Con_Printf("%s\n", svc_strings[svc_nop]); break;

        case svc_time: {
            cl.mtime[1] = cl.mtime[0];
            cl.mtime[0] = MSG_ReadFloat();
        } break;

        case svc_clientdata:    CL_ParseClientdata(MSG_ReadShort());    break;

        case svc_version: {
            int ver = MSG_ReadLong();
            if (ver != PROTOCOL_VERSION)
                Host_Error(
                    "CL_ParseServerMessage: Server is protocol %i instead of %i\n",
                    ver, PROTOCOL_VERSION
                );
        } break;

        case svc_disconnect:    Host_EndGame("Server disconnected\n");
        case svc_print:         Con_Printf("%s", MSG_ReadString());     break;
        case svc_centerprint:   SCR_CenterPrint(MSG_ReadString());      break;
        case svc_stufftext:     Cbuf_AddText(MSG_ReadString());         break;
        case svc_damage:        V_ParseDamage();                        break;

        case svc_serverinfo: {
            CL_ParseServerInfo();
            vid.recalc_refdef = true; // leave intermission full screen
        } break;

        case svc_setangle: {
            for (int i = 0; i < VECT_DIM; i++)
                cl.viewangles[i] = MSG_ReadAngle();
        } break;

        case svc_setview:       cl.viewentity = MSG_ReadShort();        break;

        case svc_lightstyle: {
            uint8_t msg = MSG_ReadByte();
            if (msg >= MAX_LIGHTSTYLES)
                Sys_Error("svc_lightstyle > MAX_LIGHTSTYLES");
            Q_strcpy(cl_lightstyle[msg].map, MSG_ReadString());
            cl_lightstyle[msg].length = Q_strlen(cl_lightstyle[msg].map);
        } break;

        case svc_sound:         CL_ParseStartSoundPacket();             break;

        case svc_stopsound: {
            int16_t msg = MSG_ReadShort();
            S_StopSound((msg >> 3), (msg & 7));
        } break;

        case svc_updatename: {
            Sbar_Changed();
            uint8_t msg = MSG_ReadByte();
            if (msg >= cl.maxclients)
                Host_Error("CL_ParseServerMessage: svc_updatename > MAX_SCOREBOARD");
            strcpy(cl.scores[msg].name, MSG_ReadString());
        } break;

        case svc_updatefrags: {
            Sbar_Changed();
            uint8_t msg = MSG_ReadByte();
            if (msg >= cl.maxclients)
                Host_Error("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
            cl.scores[msg].frags = MSG_ReadShort();
        } break;

        case svc_updatecolors: {
            Sbar_Changed();
            uint8_t msg = MSG_ReadByte();
            if (msg >= cl.maxclients)
                Host_Error("CL_ParseServerMessage: svc_updatecolors > MAX_SCOREBOARD");
            cl.scores[msg].colors = MSG_ReadByte();
            CL_NewTranslation(msg);
        }   break;

        case svc_particle:      R_ParseParticleEffect();            break;
        case svc_spawnbaseline: CL_ParseBaseline(CL_EntityNum(MSG_ReadShort()));    break; // must use CL_EntityNum() to force cl.num_entities up
        case svc_spawnstatic:   CL_ParseStatic();                   break;
        case svc_temp_entity:   CL_ParseTEnt();                     break;

        case svc_setpause: {
            if ((cl.paused = (bool)MSG_ReadByte())) CDAudio_Pause();
            else                                    CDAudio_Resume();

#ifdef _WIN32
            VID_HandlePause(cl.paused);
#endif
        }  break;

        case svc_signonnum: {
            uint8_t msg = MSG_ReadByte();
            if (msg <= cls.signon)
                Host_Error("Received signon %i when at %i", msg, cls.signon);
            cls.signon = msg;
            CL_SignonReply();
        } break;

        case svc_killedmonster:     cl.stats[STAT_MONSTERS]++; break;
        case svc_foundsecret:       cl.stats[STAT_SECRETS]++;   break;

        case svc_updatestat: {
            uint8_t msg = MSG_ReadByte();
            if (msg >= MAX_CL_STATS)
                Sys_Error("svc_updatestat: %i is invalid", msg);
            cl.stats[msg] = MSG_ReadLong();
        } break;

        case svc_spawnstaticsound:  CL_ParseStaticSound();  break;

        case svc_cdtrack:
            cl.cdtrack = MSG_ReadByte();
            cl.looptrack = MSG_ReadByte();
            if ((cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1))
                CDAudio_Play((uint8_t)cls.forcetrack, true);
            else CDAudio_Play((uint8_t)cl.cdtrack, true);
            break;

        case svc_intermission:
            cl.intermission = 1;
            cl.completed_time = cl.time;
            vid.recalc_refdef = true; // go to full screen
            break;

        case svc_finale:
            cl.intermission = 2;
            cl.completed_time = cl.time;
            vid.recalc_refdef = true; // go to full screen
            SCR_CenterPrint(MSG_ReadString());
            break;

        case svc_cutscene:
            cl.intermission = 3;
            cl.completed_time = cl.time;
            vid.recalc_refdef = true; // go to full screen
            SCR_CenterPrint(MSG_ReadString());
            break;

        case svc_sellscreen:    Cmd_ExecuteString("help", src_command); break;
        }
    }
}

