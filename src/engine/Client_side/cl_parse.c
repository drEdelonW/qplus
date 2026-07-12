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

#include "client.h"
#include "model.h"
#include "host.h"
#include <string.h>
#include <stdlib.h>
#include "sound.h"
#include "cdaudio.h"
#include "msg.h"
#include "console.h"
#include "sbar.h"
#include "cmd.h"
#include "cbuf.h"
#include "cvar_q1.h"
#include "screen.h"
#include "q_tools.h"
#include "gamedefs.h"
#ifdef GLQUAKE
#   include "qOpenGL.h"
#   include "glquake.h"
// #else
#endif
#   include "render.h"
#   include "vector_tools.h"
#include "qSymbolChar.h"
#include "view.h"



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
r_Entity_p CL_EntityNum(EdIdx num) {
    if (num >= cl.num_entities) {
        if (num >= EdictMax)      Host_Error("CL_EntityNum: %i is an invalid number", num);

        while (cl.num_entities <= num) {
            cl_entities[cl.num_entities].pColorMap = Scr.pColorMapPal;
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
    // {
    uint8_t field_mask = MSG_ReadByte();
    float volume = ((field_mask & SND_VOLUME) ? MSG_ReadByte() : VolFull) / (1.f * 255);
    float attenuation = (field_mask & SND_ATTENUATION) ? (MSG_ReadByte() / (1.f * 64)) : AtnNorm;
    int16_t channel = MSG_ReadShort();
    uint8_t sound_num = MSG_ReadByte();
    vec3_t pos = MSG_ReadVector();
    // }
    EdIdx ent = (channel >> 3);     channel &= 7; // b0111
    if (ent > EdictMax)     Host_Error("CL_ParseStartSoundPacket: ent = %i", ent);

    S_StartSound(
        ent, channel,
        cl.sound_precache[sound_num], pos,
        volume, attenuation
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
    if ((Host_IsServerActive()) || // no need if server is local
        (cls.isDemoPlaying)
        )   return;

    // read messages from server, should just be nops
    sizebuf_t old = net_message;
    netMsgBuf_t olddata;  memcpy(olddata, net_message.data, net_message.cursize);

    int ret;
    do {
        ret = CL_GetMessage();
        switch (ret) {
        default:    Host_Error("CL_KeepaliveMessage: CL_GetMessage failed");    break;
        case 0:                                                                 break; // nothing waiting
        case 1:     Host_Error("CL_KeepaliveMessage: received a message");      break;
        case 2: {
            if (MSG_ReadByte() != svc_nop)
                Host_Error("CL_KeepaliveMessage: datagram wasn't a nop");
        } break;
        }
    } while (ret);

    net_message = old;
    memcpy(net_message.data, olddata, net_message.cursize);

    // check time
    LegDt_t time = (LegDt_t)Host_FloatTime();
    static LegDt_t _lastMsg;
    if ((time - _lastMsg) < 5.0f)    return;
    _lastMsg = time;

    // write out a nop
    Con_Printf("--> client to server keepalive\n");

    MSG_WriteByte(&cls.message, clc_nop);    NET_SendMessage(cls.netcon, &cls.message);
    SZ_Clear(&cls.message);
}

#include "z_hunk.h"
/*
    ==================
    CL_ParseServerInfo
    ==================
*/
void CL_ParseServerInfo() {
    Con_DPrintf("Serverinfo packet received.\n");

    CL_ClearState();    // wipe the ClientState_t struct

    // parse protocol version number
    int32_t ver = MSG_ReadLong();
    if (ver != PROTOCOL_VERSION) {
        Con_Printf("Server returned version %i, not %i", ver, PROTOCOL_VERSION);
        return;
    }

    // parse maxClients
    cl.maxclients = MSG_ReadByte();
    if ((cl.maxclients < 1) ||
        (cl.maxclients > MAX_SCOREBOARD)
        ) {
        Con_Printf("Bad maxClients (%u) from server\n", cl.maxclients);
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
    qPathStr_t model_precache[MAX_MODELS];
    memset(cl.model_precache, 0, sizeof(cl.model_precache));
    uint16_t nummodels;
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
    qPathStr_t sound_precache[MAX_SOUNDS];
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
static int _bitCnt[16]; // FYI: DEBUG metrics

void CL_ParseUpdate(update_bits_t bits) {
    if (cls.signon == SIGNONS - 1) { // first update is the final signon stage
        cls.signon = SIGNONS;
        CL_SignonReply();
    }

    if (bits & U_MOREBITS) {
        uint32_t ext = MSG_ReadByte();
        if (getMsgBadRead()) { Host_Error("CL_ParseUpdate: bad MOREBITS"); }
        bits |= (ext) << 8;
    }

    int num = (bits & U_LONGENTITY) ? MSG_ReadShort() : MSG_ReadByte();
    r_Entity_p ent = CL_EntityNum(num);

    for (int i = 0; i < 16; i++) {
        if (bits & (1u << i))
            _bitCnt[i]++;
    }

    bool forcelink = (ent->msgtime != cl.mtime[Prev]); // no previous frame to lerp from

    ent->msgtime = cl.mtime[Cur];

    uint16_t modnum = (bits & U_MODEL) ? (uint16_t)MSG_ReadByte() : (uint16_t)ent->baseline.modelindex;
    if (modnum >= MAX_MODELS)   Host_Error("CL_ParseModel: bad modnum");

    Model_p model = cl.model_precache[modnum];
    if (model != ent->model) {
        ent->model = model;
        // automatic animation (torches, etc) can be either all together
        // or randomized
        if (model)  ent->syncbase = (model->synctype == ST_RAND) ? ((float)(rand() & 0x7fff) / 0x7fff) : 0.0f;
        else        forcelink = true; // hack to make null model players work

#ifdef GLQUAKE
        if ((num > 0) &&
            (num <= cl.maxclients)
            )   R_TranslatePlayerSkin(num - 1);
#endif
    }

    ent->frame = (bits & U_FRAME) ? MSG_ReadByte() : ent->baseline.frame;
    uint8_t i = (bits & U_COLORMAP) ? MSG_ReadByte() : (uint8_t)ent->baseline.colormap;

    if (!i)     ent->pColorMap = Scr.pColorMapPal;
    else {
        if (i > cl.maxclients)  Host_SysError("i >= cl.maxclients %d > %d", i, cl.maxclients);

        ent->pColorMap = &cl.scores[i - 1].translations;
    }

#ifdef GLQUAKE
    int skin = (bits & U_SKIN) ? MSG_ReadByte() : ent->baseline.skin;

    if (skin != ent->skinnum) {
        ent->skinnum = skin;
        if ((num > 0) &&
            (num <= cl.maxclients)
            )   R_TranslatePlayerSkin(num - 1);
    }

#else
    ent->skinnum = (bits & U_SKIN) ? MSG_ReadByte() : ent->baseline.skin;
#endif
    ent->effects = (bits & U_EFFECTS) ? MSG_ReadByte() : ent->baseline.effects;

    // shift the known values for interpolation
    ent->msgPoses[Prev].loc = ent->msgPoses[Cur].loc;
    ent->msgPoses[Prev].aim = ent->msgPoses[Cur].aim;

    ent->msgPoses[Cur].loc.x = (bits & U_ORIGIN1) ? MSG_ReadCoord() : ent->baseline.pose.loc.x;
    ent->msgPoses[Cur].aim.pitch = (bits & U_ANGLE1) ? MSG_ReadAngle() : ent->baseline.pose.aim.pitch;
    ent->msgPoses[Cur].loc.y = (bits & U_ORIGIN2) ? MSG_ReadCoord() : ent->baseline.pose.loc.y;
    ent->msgPoses[Cur].aim.yaw = (bits & U_ANGLE2) ? MSG_ReadAngle() : ent->baseline.pose.aim.yaw;
    ent->msgPoses[Cur].loc.z = (bits & U_ORIGIN3) ? MSG_ReadCoord() : ent->baseline.pose.loc.z;
    ent->msgPoses[Cur].aim.roll = (bits & U_ANGLE3) ? MSG_ReadAngle() : ent->baseline.pose.aim.roll;

    if (bits & U_NOLERP)
        ent->forcelink = true;

    if (forcelink) { // didn't have an update last message
        ent->msgPoses[Prev].loc = ent->msgPoses[Cur].loc;
        ent->pose.loc = ent->msgPoses[Cur].loc;
        ent->msgPoses[Prev].aim = ent->msgPoses[Cur].aim;
        ent->pose.aim = ent->msgPoses[Cur].aim;
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
    for (int i = 0; i < VECT_DIM; i++) {    // TODO: wrap MSG_ReadCoord/MSG_ReadAngle to MSG_vector_tools
        ent->baseline.pose.loc.v[i] = MSG_ReadCoord();
        ent->baseline.pose.aim.v[i] = MSG_ReadAngle();
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

    cl.mvelocity[Prev] = cl.mvelocity[Cur];
    for (int i = 0; i < VECT_DIM; i++) {    // TODO: wrap MSG_ReadChar to MSG_vector_tools
        cl.punchangle.v[i] = (bits & (SU_PUNCH1 << i)) ? (MSG_ReadChar() * 1.f) : 0.f;
        cl.mvelocity[Cur].v[i] = (bits & (SU_VELOCITY1 << i)) ? fixed4_tof(MSG_ReadChar()) : 0.f;
    }
    uint32_t msg;
    // [always sent]    SU_ITEMS
    if (bits & SU_ITEMS) {
        msg = (uint32_t)MSG_ReadLong();
        if (cl.items != msg) { // set flash times
            upd = true; /* Sbar_Changed(); */
            for (int i = 0; i < 32; i++)
                if ((msg & (1u << i)) &&
                    !(cl.items & (1u << i))
                    )   cl.item_gettime[i] = GetClSimTime();
            cl.items = msg;
        }
    }

    cl.onground = (bits & SU_ONGROUND) != 0;
    cl.inwater = (bits & SU_INWATER) != 0;

    cl.stats[STAT_WEAPONFRAME] = (bits & SU_WEAPONFRAME) ? MSG_ReadByte() : 0;

    msg = (bits & SU_ARMOR) ? MSG_ReadByte() : 0;   if (cl.stats[STAT_ARMOR] != msg) { cl.stats[STAT_ARMOR] = msg; upd = true; /* Sbar_Changed(); */ }
    msg = (bits & SU_WEAPON) ? MSG_ReadByte() : 0;  if (cl.stats[STAT_WEAPON] != msg) { cl.stats[STAT_WEAPON] = msg; upd = true; /* Sbar_Changed(); */ }

    msg = (uint32_t)MSG_ReadShort();  if (cl.stats[STAT_HEALTH] != msg) { cl.stats[STAT_HEALTH] = msg; upd = true; /* Sbar_Changed(); */ }
    msg = (uint32_t)MSG_ReadByte();   if (cl.stats[STAT_AMMO] != msg) { cl.stats[STAT_AMMO] = msg; upd = true; /* Sbar_Changed(); */ }

    for (int i = 0; i < 4; i++) {
        uint32_t msg = MSG_ReadByte(); if (cl.stats[STAT_SHELLS + i] != msg) { cl.stats[STAT_SHELLS + i] = msg; upd = true; /* Sbar_Changed(); */ }
    }

    msg = (uint32_t)MSG_ReadByte();
    uint32_t w = (standard_quake) ? msg : (1U << msg);
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
        (slot > cl.maxclients)
        )   Host_SysError("CL_NewTranslation: bad slot %d (max %d)", slot, cl.maxclients);


    cl.scores[slot].translations = *Scr.pColorMapPal;

    qColor8_p dest = cl.scores[slot].translations.raw;  // TODO: rework next skin translation
    qColor8_p source = Scr.pColorMapPal->raw;

    int top = (cl.scores[slot].colors & 0xF0);       // tshort
    int bottom = (cl.scores[slot].colors & 0x0F) << 4;  // pents
#ifdef GLQUAKE
    R_TranslatePlayerSkin(slot);
#endif

    for (int i = 0; i < VID_GRADES; i++, dest += InksNum, source += InksNum) {
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
    ent->pColorMap = Scr.pColorMapPal;
    ent->skinnum = ent->baseline.skin;
    ent->effects = ent->baseline.effects;

    ent->pose = ent->baseline.pose;
    R_AddEfrags(ent);
}

/*
    ===================
    CL_ParseStaticSound
    ===================
*/
void CL_ParseStaticSound() {
    vec3_t org = MSG_ReadVector();
    uint8_t sound_num = MSG_ReadByte();
    uint8_t vol = MSG_ReadByte();
    uint8_t atten = MSG_ReadByte();

    S_StaticSound(cl.sound_precache[sound_num], org, vol, atten);
}


#define SHOWNET(x) if (cl_shownet.value == 2) { Con_Printf("%3i:%s\n", (getMsgReadCount() - 1), x);}
static inline void ShowNet(cStringRO x) {
    if (cl_shownet.value == 2)
        Con_Printf("%3i:%s\n", (getMsgReadCount() - 1), x);
}
/*
    =====================
    CL_ParseServerMessage
    =====================
*/
#ifdef _WIN32
# include "vid.h"    // VID_HandlePause();
#endif
void CL_ParseServerMessage() {
    // if recording demos, copy the message out
    /**/ if (cl_shownet.value == 1) Con_Printf("%i ", net_message.cursize);
    else if (cl_shownet.value == 2) Con_Printf("------------------\n");

    cl.onground = false; // unless the server says otherwise parse the message
    MSG_BeginReading();

    while (1) {
        if (getMsgBadRead())    Host_Error("CL_ParseServerMessage: Bad server message");

        svc_t cmd = MSG_ReadByte();

        if (getMsgBadRead()) {
            ShowNet("END OF MESSAGE");
            return;  // end of message
        }

        // if the high bit of the command byte is set, it is a fast update
        if (cmd & svc_fastFlag) {
            ShowNet("fast update");
            CL_ParseUpdate(cmd & svc_fastMask);
            continue; // while
        }

        ShowNet(svc_strings[cmd]);

        // other commands
        // int msg;
        switch (cmd) {
        default:    Host_Error("CL_ParseServerMessage: Illegible server message [0x%X]\n", cmd); break;

        case svc_nop:       Con_Printf("%s\n", svc_strings[svc_nop]); break;

        case svc_time: {    // timestamp rotation
            cl.mtime[Prev] = cl.mtime[Cur];
            cl.mtime[Cur] = MSG_ReadFloat();
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
            SCR_RequestCalcRefdef(); // leave intermission full screen
        } break;

        case svc_setangle:      cl.viewangles = MSG_ReadAngles();       break;
        case svc_setview:       cl.viewentity = MSG_ReadShort();        break;

        case svc_lightstyle: {
            uint8_t msg = MSG_ReadByte();
            if (msg >= MAX_LIGHTSTYLES)     Host_SysError("svc_lightstyle > MAX_LIGHTSTYLES");

            Q_strcpy(cl_lightstyle[msg].map, MSG_ReadString());
            cl_lightstyle[msg].length = Q_strlen(cl_lightstyle[msg].map);
        } break;

        case svc_sound:         CL_ParseStartSoundPacket();             break;

        case svc_stopsound: {
            int16_t msg = MSG_ReadShort();
            S_StopSound(DIV4(msg), (msg & 0x07));
        } break;

        case svc_updatename: {
            Sbar_Changed();
            uint8_t msg = MSG_ReadByte();
            if (msg >= cl.maxclients)       Host_Error("CL_ParseServerMessage: svc_updatename > MAX_SCOREBOARD");

            strcpy(cl.scores[msg].name, MSG_ReadString());
        } break;

        case svc_updatefrags: {
            Sbar_Changed();
            uint8_t msg = MSG_ReadByte();
            if (msg >= cl.maxclients)       Host_Error("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");

            cl.scores[msg].frags = MSG_ReadShort();
        } break;

        case svc_updatecolors: {
            Sbar_Changed();
            uint8_t msg = MSG_ReadByte();
            if (msg >= cl.maxclients)       Host_Error("CL_ParseServerMessage: svc_updatecolors > MAX_SCOREBOARD");

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
            if (msg <= cls.signon)          Host_Error("Received signon %i when at %i", msg, cls.signon);

            cls.signon = msg;
            CL_SignonReply();
        } break;

        case svc_killedmonster:     cl.stats[STAT_MONSTERS]++; break;
        case svc_foundsecret:       cl.stats[STAT_SECRETS]++;   break;

        case svc_updatestat: {
            uint8_t msg = MSG_ReadByte();
            if (msg >= MAX_CL_STATS)        Host_SysError("svc_updatestat: %i is invalid", msg);

            cl.stats[msg] = MSG_ReadLong();
        } break;

        case svc_spawnstaticsound:  CL_ParseStaticSound();  break;

        case svc_cdtrack: {
            cl.cdtrack = MSG_ReadByte();
            cl.looptrack = MSG_ReadByte();
            if ((cls.isDemoPlaying ||
                cls.demorecording) &&
                (cls.forcetrack != -1)
                )
                CDAudio_Play((uint8_t)cls.forcetrack, true);
            else CDAudio_Play((uint8_t)cl.cdtrack, true);
        } break;

        case svc_intermission: {
            cl.intermission = IM_LEVEL;
            cl.completed_time = (uint32_t)GetClSimTime();
            SCR_RequestCalcRefdef(); // go to full screen
        } break;

        case svc_finale: {
            cl.intermission = IM_FINALE;
            cl.completed_time = (uint32_t)GetClSimTime();
            SCR_RequestCalcRefdef(); // go to full screen
            SCR_CenterPrint(MSG_ReadString());
        } break;

        case svc_cutscene: {
            cl.intermission = IM_CUTSCENE;
            cl.completed_time = (uint32_t)GetClSimTime();
            SCR_RequestCalcRefdef(); // go to full screen
            SCR_CenterPrint(MSG_ReadString());
        } break;

        case svc_sellscreen:    Cmd_ExecuteString("help", src_command); break;
        }
    }
}

