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
// sv_main.c -- server main program

#include "server.h"
#undef SERVER
#include "client.h"
#include <string.h>
#include "versions.h"
#include "common.h"
#include "screen.h"
#include "sys.h"
#include "msg.h"
#include "q_tools.h"
#include "cvar_q1.h"
#include "protocol.h"
#include "cmd.h"
#include "sound.h"
#include "world.h"
#include "console.h"
#include "host.h"
#include "net.h"
#include "render.h"
#include "progs.h"

server_t    sv;
sv_static_t svs;
RmtClient_p remoteClient;   // current client
int32_t     current_skill;
static char _localModels[MAX_MODELS][5];    // inline model names for precache

//============================================================================

/*
    ===============
    SV_Init
    ===============
*/
void SV_Init() {
    Cvar_RegisterVariable(&sv_maxvelocity);
    Cvar_RegisterVariable(&sv_gravity);
    Cvar_RegisterVariable(&sv_friction);
    Cvar_RegisterVariable(&sv_edgefriction);
    Cvar_RegisterVariable(&sv_stopspeed);
    Cvar_RegisterVariable(&sv_maxspeed);
    Cvar_RegisterVariable(&sv_accelerate);
    Cvar_RegisterVariable(&sv_idealpitchscale);
    Cvar_RegisterVariable(&sv_aim);
    Cvar_RegisterVariable(&sv_nostep);

    for (int i = 0; i < MAX_MODELS; i++) {
        snprintf(_localModels[i], sizeof(_localModels[i]), "*%i", i);
    }
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
    ==================
    SV_StartParticle

    Make sure the event gets sent to all clients
    ==================
*/
void SV_StartParticle(vec3_t org, vec3_t dir, int color, size_t count) {
    if (sv.datagram.cursize > (MAX_DATAGRAM - 16))  return;

    MSG_WriteByte(&sv.datagram, svc_particle);
    MSG_WriteCoord(&sv.datagram, org[0]);
    MSG_WriteCoord(&sv.datagram, org[1]);
    MSG_WriteCoord(&sv.datagram, org[2]);
    for (int i = 0; i < VECT_DIM; i++) {
        int v = (int)(dir[i] * 16.0f);
        CLAMP(-128, v, 127);
        MSG_WriteChar(&sv.datagram, (int8_t)v);
    }
    MSG_WriteByte(&sv.datagram, (uint8_t)count);
    MSG_WriteByte(&sv.datagram, (uint8_t)color);
}

/*
    ==================
    SV_StartSound

    Each entity can have eight independant sound sources, like voice,
    weapon, feet, etc.

    Channel 0 is an auto-allocate channel, the others override anything
    allready running on that entity/channel pair.

    An attenuation of 0 will play full volume everywhere in the level.
    Larger attenuations will drop off.  (max 4 attenuation)

    ==================
*/
void SV_StartSound(edict_p entity, int channel, cString sample, int volume, float attenuation) {
    if ((volume < 0) || (volume > 255))         Sys_Error("SV_StartSound: volume = %i", volume);
    if ((attenuation < 0) || (attenuation > 4)) Sys_Error("SV_StartSound: attenuation = %f", attenuation);
    if ((channel < 0) || (channel > 7))         Sys_Error("SV_StartSound: channel = %i", channel);

    if (sv.datagram.cursize > (MAX_DATAGRAM - 16))  return;

    // find precache number for sound
    int sound_num = 1;
    for (;
        (sound_num < MAX_SOUNDS) &&
        sv.sound_precache[sound_num];
        sound_num++) {
        if (!strcmp(sample, sv.sound_precache[sound_num]))
            break;
    }
    if ((sound_num == MAX_SOUNDS) ||
        (!sv.sound_precache[sound_num])) {
        Con_Printf("SV_StartSound: %s not precacheed\n", sample);
        return;
    }

    channel |= (int)(NUM_FOR_EDICT(entity) << 3);

    int field_mask = 0x00;
    if (volume != DEFAULT_SOUND_PACKET_VOLUME)              field_mask |= SND_VOLUME;
    if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)    field_mask |= SND_ATTENUATION;

    // directed messages go only to the entity the are targeted on
    MSG_WriteByte(&sv.datagram, svc_sound);    MSG_WriteByte(&sv.datagram, (uint8_t)field_mask);
    if (field_mask & SND_VOLUME)        MSG_WriteByte(&sv.datagram, (uint8_t)volume);
    if (field_mask & SND_ATTENUATION)   MSG_WriteByte(&sv.datagram, (uint8_t)(attenuation * 64));
    MSG_WriteShort(&sv.datagram, (int16_t)channel);
    MSG_WriteByte(&sv.datagram, (uint8_t)sound_num);
    for (int i = 0; i < VECT_DIM; i++) {
        MSG_WriteCoord(
            &sv.datagram,
            entity->v.origin[i] +
            0.5f * (entity->v.mins[i] + entity->v.maxs[i])
        );
    }
}

/*
    ==============================================================================

    CLIENT SPAWNING

    ==============================================================================
*/

/*
    ================
    SV_SendServerinfo

    Sends the first message from the server to a connected client.
    This will be sent on the initial connection and upon each server load.
    ================
*/
void SV_SendServerinfo(RmtClient_p client) {
    char message[2048];

    sizebuf_p pBuf = &client->message;
    MSG_WriteByte(pBuf, svc_print);
    snprintf(message, sizeof(message), "%c\nVERSION %4.2f SERVER (%i CRC)", 2, VERSION, pr_crc);
    MSG_WriteString(pBuf, message);

    MSG_WriteByte(pBuf, svc_serverinfo);    MSG_WriteLong(pBuf, PROTOCOL_VERSION);    MSG_WriteByte(pBuf, svs.maxClients);

    MSG_WriteByte(pBuf, (!coop.value && deathmatch.value) ? GAME_DEATHMATCH : GAME_COOP);

    snprintf(message, sizeof(message), "%s", pr_strings + sv.edicts->v.message);

    MSG_WriteString(pBuf, message);

    for (cStringArray s = (sv.model_precache + 1); *s; s++)
        MSG_WriteString(pBuf, *s);
    MSG_WriteByte(pBuf, 0);

    for (cStringArray s = (sv.sound_precache + 1); *s; s++)
        MSG_WriteString(pBuf, *s);
    MSG_WriteByte(pBuf, 0);

    // send music
    MSG_WriteByte(pBuf, svc_cdtrack);
    MSG_WriteByte(pBuf, (uint8_t)sv.edicts->v.sounds);
    MSG_WriteByte(pBuf, (uint8_t)sv.edicts->v.sounds);

    // set view
    MSG_WriteByte(pBuf, svc_setview);
    MSG_WriteShort(pBuf, (int16_t)NUM_FOR_EDICT(client->edict));

    MSG_WriteByte(pBuf, svc_signonnum);
    MSG_WriteByte(pBuf, 1);

    client->sendsignon = true;
    client->spawned = false;    // need prespawn, spawn, etc
}

/*
    ================
    SV_ConnectClient

    Initializes a RmtClient_t for a new net connection.  This will only be called
    once for a player each game, not once for each level change.
    ================
*/
void SV_ConnectClient(uint32_t clientnum) {
    float spawn_parms[NUM_SPAWN_PARMS];

    RmtClient_p client = svs.clients + clientnum;
    Con_DPrintf("Client %s connected\n", client->netconnection->address);

    // set up the RmtClient_t
    qsocket_p netconnection = client->netconnection;

    if (sv.loadgame)
        memcpy(spawn_parms, client->spawn_parms, sizeof(spawn_parms));
    memset(client, 0, sizeof(*client));
    client->netconnection = netconnection;

    strcpy(client->name, "unconnected");
    client->active = true;
    client->spawned = false;
    client->edict = EDICT_NUM(clientnum + 1);
    client->message.data = client->msgbuf;
    client->message.maxsize = sizeof(client->msgbuf);
    client->message.allowoverflow = true;    // we can catch it

    client->privileged =
#ifdef IDGODS
        IsID(&client->netconnection->addr);
#else
        false;
#endif

    if (sv.loadgame)
        memcpy(client->spawn_parms, spawn_parms, sizeof(spawn_parms));
    else {
        // call the progs to get default spawn parms for the new client
        PR_ExecuteProgram(pr_global_struct->SetNewParms);
        for (int i = 0; i < NUM_SPAWN_PARMS; i++)
            client->spawn_parms[i] = (&pr_global_struct->parm1)[i];
    }

    SV_SendServerinfo(client);
}


/*
    ===================
    SV_CheckForNewClients

    ===================
*/
void SV_CheckForNewClients() {
    //
    // check for new connections
    //
    while (1) {
        qsocket_p ret = NET_CheckNewConnections();
        if (!ret)
            break;

        //
        // init a new client structure
        //
        uint32_t i = 0;
        for (; i < svs.maxClients; i++) {
            if (!svs.clients[i].active) break;
        }
        if (i == svs.maxClients)        Sys_Error("Host_CheckForNewClients: no free clients");

        svs.clients[i].netconnection = ret;
        SV_ConnectClient(i);

        net_activeconnections++;
    }
}



/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
    ==================
    SV_ClearDatagram

    ==================
*/
void SV_ClearDatagram() { SZ_Clear(&sv.datagram); }

/*
    =============================================================================

    The PVS must include a small area around the client to allow head bobbing
    or other small motion on the client side.  Otherwise, a bob might cause an
    entity that should be visible to not show up, especially when the bob
    crosses a waterline.

    =============================================================================
*/

static uint32_t  _fatBytes;
static uint8_t _fatPvs[MAX_MAP_LEAFS / 8];

void SV_AddToFatPVS(vec3_t org, mNode_p node) {
    while (1) {
        // if this is a leaf, accumulate the pvs bits
        if (node->contents < 0) {
            if (node->contents != CONTENTS_SOLID) {
                uint8_p pvs = Mod_LeafPVS((mLeaf_t*)node, sv.worldmodel);
                for (int i = 0; i < _fatBytes; i++) {
                    _fatPvs[i] |= pvs[i];
                }
            }
            return;
        }

        mPlane_p plane = node->plane;
        float d = DotProduct(org, plane->normal) - plane->dist;
        if (d > 8)          node = node->children[0];
        else if (d < -8)    node = node->children[1];
        else {  // go down both
            SV_AddToFatPVS(org, node->children[0]);
            node = node->children[1];
        }
    }
}

/*
    =============
    SV_FatPVS

    Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
    given point.
    =============
*/
uint8_p SV_FatPVS(vec3_t org) {
    _fatBytes = (sv.worldmodel->numleafs + 31) >> 3;
    Q_memset(_fatPvs, 0, _fatBytes);
    SV_AddToFatPVS(org, sv.worldmodel->nodes);
    return _fatPvs;
}

//=============================================================================


/*
    =============
    SV_WriteEntitiesToClient

    =============
*/
void SV_WriteEntitiesToClient(edict_p clent, sizebuf_p msg) {
    // find the client's PVS
    vec3_t  org;    VectorAdd(clent->v.origin, clent->v.view_ofs, org);
    uint8_p pvs = SV_FatPVS(org);

    // send over all entities (excpet the client) that touch the pvs
    edict_p ent = NEXT_EDICT(sv.edicts);
    for (int e = 1; e < sv.num_edicts; e++, ent = NEXT_EDICT(ent)) {
#ifdef QUAKE2
        // don't send if flagged for NODRAW and there are no lighting effects
        if (ent->v.effects == EF_NODRAW)
            continue;
#endif

        // ignore if not touching a PV leaf
        if (ent != clent) {  // clent is ALLWAYS sent
            // ignore ents without visible models
            if (!ent->v.modelindex || !pr_strings[ent->v.model])    continue;

            int i = 0;
            for (; i < ent->num_leafs; i++)
                if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i] & 7))) break;

            if (i == ent->num_leafs) continue;    // not visible
        }

        if (msg->maxsize - msg->cursize < 16) { Con_Printf("packet overflow\n"); return; }

        // send an update
        uint16_t bits = 0;

        for (int i = 0; i < VECT_DIM; i++) {
            float miss = ent->v.origin[i] - ent->baseline.origin[i];
            if ((miss < -0.1) || (miss > 0.1))              bits |= (U_ORIGIN1 << i);
        }

        if (ent->v.angles[0] != ent->baseline.angles[0])    bits |= U_ANGLE1;
        if (ent->v.angles[1] != ent->baseline.angles[1])    bits |= U_ANGLE2;
        if (ent->v.angles[2] != ent->baseline.angles[2])    bits |= U_ANGLE3;
        if (ent->v.movetype == MOVETYPE_STEP)               bits |= U_NOLERP;  // don't mess up the step animation
        if (ent->v.colormap != ent->baseline.colormap)      bits |= U_COLORMAP;
        if (ent->v.skin != ent->baseline.skin)              bits |= U_SKIN;
        if (ent->v.frame != ent->baseline.frame)            bits |= U_FRAME;
        if (ent->v.effects != ent->baseline.effects)        bits |= U_EFFECTS;
        if (ent->v.modelindex != ent->baseline.modelindex)  bits |= U_MODEL;
        if (e >= 256)                                       bits |= U_LONGENTITY;
        if (bits >= 256)                                    bits |= U_MOREBITS;

        //
        // write the message
        //
        MSG_WriteByte(msg, (uint8_t)(bits | U_SIGNAL));

        if (bits & U_MOREBITS)  MSG_WriteByte(msg, (uint8_t)(bits >> 8));
        if (bits & U_LONGENTITY)MSG_WriteShort(msg, (int16_t)e);
        else                    MSG_WriteByte(msg, (uint8_t)e);

        if (bits & U_MODEL)     MSG_WriteByte(msg, (uint8_t)ent->v.modelindex);
        if (bits & U_FRAME)     MSG_WriteByte(msg, (uint8_t)ent->v.frame);
        if (bits & U_COLORMAP)  MSG_WriteByte(msg, (uint8_t)ent->v.colormap);
        if (bits & U_SKIN)      MSG_WriteByte(msg, (uint8_t)ent->v.skin);
        if (bits & U_EFFECTS)   MSG_WriteByte(msg, (uint8_t)ent->v.effects);
        if (bits & U_ORIGIN1)   MSG_WriteCoord(msg, ent->v.origin[0]);
        if (bits & U_ANGLE1)    MSG_WriteAngle(msg, ent->v.angles[0]);
        if (bits & U_ORIGIN2)   MSG_WriteCoord(msg, ent->v.origin[1]);
        if (bits & U_ANGLE2)    MSG_WriteAngle(msg, ent->v.angles[1]);
        if (bits & U_ORIGIN3)   MSG_WriteCoord(msg, ent->v.origin[2]);
        if (bits & U_ANGLE3)    MSG_WriteAngle(msg, ent->v.angles[2]);
    }
}

/*
    =============
    SV_CleanupEnts

    =============
*/
void SV_CleanupEnts() {
    edict_p ent = NEXT_EDICT(sv.edicts);
    for (int e = 1; e < sv.num_edicts; e++, ent = NEXT_EDICT(ent)) {
        ent->v.effects = (int)ent->v.effects & ~EF_MUZZLEFLASH;
    }
}

/*
    ==================
    SV_WriteClientdataToMessage

    ==================
*/
void SV_WriteClientdataToMessage(edict_p ent, sizebuf_p msg) {
    //
    // send a damage message
    //
    if (ent->v.dmg_take || ent->v.dmg_save) {
        edict_p other = PROG_TO_EDICT(ent->v.dmg_inflictor);
        MSG_WriteByte(msg, svc_damage);
        MSG_WriteByte(msg, (uint8_t)ent->v.dmg_save);
        MSG_WriteByte(msg, (uint8_t)ent->v.dmg_take);
        for (int i = 0; i < VECT_DIM; i++)
            MSG_WriteCoord(msg,
                other->v.origin[i] +
                0.5f *
                (other->v.mins[i] +
                    other->v.maxs[i])
            );

        ent->v.dmg_take = 0;
        ent->v.dmg_save = 0;
    }

    //
    // send the current viewpos offset from the view entity
    //
    SV_SetIdealPitch();    // how much to look up / down ideally

    // a fixangle might get lost in a dropped packet.  Oh well.
    if (ent->v.fixangle) {
        MSG_WriteByte(msg, svc_setangle);
        for (int i = 0; i < VECT_DIM; i++)
            MSG_WriteAngle(msg, ent->v.angles[i]);
        ent->v.fixangle = 0;
    }

    int bits = 0;
    if (ent->v.view_ofs[2] != DEFAULT_VIEWHEIGHT)   bits |= SU_VIEWHEIGHT;
    if (ent->v.idealpitch)                          bits |= SU_IDEALPITCH;

    // stuff the sigil bits into the high bits of items for sbar, or else
    // mix in items2
#ifdef QUAKE2
    int items = (int)ent->v.items | ((int)ent->v.items2 << 23);
#else
    eval_p val = GetEdictFieldValue(ent, "items2");
    int items = (int)ent->v.items | (
        (val) ?
        ((int)val->_float << 23) :
        ((int)pr_global_struct->serverflags << 28)
        );
#endif

    bits |= SU_ITEMS;

    if ((int)ent->v.flags & FL_ONGROUND)    bits |= SU_ONGROUND;
    if (ent->v.waterlevel >= 2)             bits |= SU_INWATER;
    for (int i = 0; i < VECT_DIM; i++) {
        if (ent->v.punchangle[i])           bits |= (SU_PUNCH1 << i);
        if (ent->v.velocity[i])             bits |= (SU_VELOCITY1 << i);
    }
    if (ent->v.weaponframe)                 bits |= SU_WEAPONFRAME;
    if (ent->v.armorvalue)                  bits |= SU_ARMOR;
    if (ent->v.weapon)                      bits |= SU_WEAPON;

    // send the data

    MSG_WriteByte(msg, svc_clientdata);
    MSG_WriteShort(msg, (int16_t)bits);
    if (bits & SU_VIEWHEIGHT)           MSG_WriteChar(msg, (int8_t)ent->v.view_ofs[2]);
    if (bits & SU_IDEALPITCH)           MSG_WriteChar(msg, (int8_t)ent->v.idealpitch);
    for (int i = 0; i < VECT_DIM; i++) {
        if (bits & (SU_PUNCH1 << i))      MSG_WriteChar(msg, (int8_t)ent->v.punchangle[i]);
        if (bits & (SU_VELOCITY1 << i))   MSG_WriteChar(msg, (int8_t)ent->v.velocity[i] / 16);
    }

    // [always sent]
    /* if (bits & SU_ITEMS) */          MSG_WriteLong(msg, items);

    if (bits & SU_WEAPONFRAME)          MSG_WriteByte(msg, (uint8_t)ent->v.weaponframe);
    if (bits & SU_ARMOR)                MSG_WriteByte(msg, (uint8_t)ent->v.armorvalue);
    if (bits & SU_WEAPON)               MSG_WriteByte(msg, (uint8_t)SV_ModelIndex(pr_strings + ent->v.weaponmodel));

    MSG_WriteShort(msg, (int16_t)ent->v.health);
    MSG_WriteByte(msg, (uint8_t)ent->v.currentammo);
    MSG_WriteByte(msg, (uint8_t)ent->v.ammo_shells);
    MSG_WriteByte(msg, (uint8_t)ent->v.ammo_nails);
    MSG_WriteByte(msg, (uint8_t)ent->v.ammo_rockets);
    MSG_WriteByte(msg, (uint8_t)ent->v.ammo_cells);

    if (standard_quake) {
        MSG_WriteByte(msg, (uint8_t)ent->v.weapon);
    }
    else {
        for (uint8_t i = 0; i < 32; i++) {
            if (((int)ent->v.weapon) & (1 << i)) {
                MSG_WriteByte(msg, i);
                break;
            }
        }
    }
}

/*
    =======================
    SV_SendClientDatagram
    =======================
*/
bool SV_SendClientDatagram(RmtClient_t* client) {
    uint8_t    buf[MAX_DATAGRAM];
    sizebuf_t   msg = {
        .data = buf,
        .maxsize = sizeof(buf),
        .cursize = 0
    };

    // msg.data = buf;
    // msg.maxsize = sizeof(buf);
    // msg.cursize = 0;

    MSG_WriteByte(&msg, svc_time);
    MSG_WriteFloat(&msg, (float)sv.time);

    // add the client specific data to the datagram
    SV_WriteClientdataToMessage(client->edict, &msg);
    SV_WriteEntitiesToClient(client->edict, &msg);

    // copy the server datagram if there is space
    if (msg.cursize + sv.datagram.cursize < msg.maxsize)
        SZ_Write(&msg, sv.datagram.data, sv.datagram.cursize);

    // send the datagram
    if (NET_SendUnreliableMessage(client->netconnection, &msg) == -1) {
        SV_DropClient(true);// if the message couldn't send, kick off
        return false;
    }

    return true;
}

/*
    =======================
    SV_UpdateToReliableMessages
    =======================
*/
void SV_UpdateToReliableMessages() {
    RmtClient_p client;

    // check for changes to be sent over the reliable streams
    remoteClient = svs.clients;
    for (int i = 0; i < svs.maxClients; i++, remoteClient++) {
        if (remoteClient->old_frags != remoteClient->edict->v.frags) {
            client = svs.clients;
            for (int j = 0; j < svs.maxClients; j++, client++) {
                if (!client->active)    continue;

                sizebuf_p pBuf = &client->message;
                MSG_WriteByte(pBuf, svc_updatefrags);   MSG_WriteByte(pBuf, (uint8_t)i); MSG_WriteShort(pBuf, (int16_t)remoteClient->edict->v.frags);
            }

            remoteClient->old_frags = (int16_t)remoteClient->edict->v.frags;
        }
    }

    client = svs.clients;
    for (int j = 0; j < svs.maxClients; j++, client++) {
        if (!client->active)    continue;
        SZ_Write(&client->message, sv.reliable_datagram.data, sv.reliable_datagram.cursize);
    }

    SZ_Clear(&sv.reliable_datagram);
}


/*
    =======================
    SV_SendNop

    Send a nop message without trashing or sending the accumulated client
    message buffer
    =======================
*/
void SV_SendNop(RmtClient_p client) {
    uint8_t    buf[4];
    sizebuf_t  msg = {
        .data = buf,
        .maxsize = sizeof(buf),
        .cursize = 0,
    };

    MSG_WriteChar(&msg, svc_nop);

    if (NET_SendUnreliableMessage(client->netconnection, &msg) == -1)
        SV_DropClient(true);  // if the message couldn't send, kick off
    client->last_message = realtime;
}

/*
    =======================
    SV_SendClientMessages
    =======================
*/
void SV_SendClientMessages() {
    // update frags, names, etc
    SV_UpdateToReliableMessages();

    // build individual updates
    remoteClient = svs.clients;
    for (int i = 0; i < svs.maxClients; i++, remoteClient++) {
        if (!remoteClient->active)   continue;

        if (remoteClient->spawned) {
            if (!SV_SendClientDatagram(remoteClient))   continue;
        }
        else {
            // the player isn't totally in the game yet
            // send small keepalive messages if too much time has passed
            // send a full message when the next signon stage has been requested
            // some other message data (name changes, etc) may accumulate
            // between signon stages
            if (!remoteClient->sendsignon) {
                if (realtime - remoteClient->last_message > 5)
                    SV_SendNop(remoteClient);
                continue;  // don't send out non-signon messages
            }
        }

        // check for an overflowed message.  Should only happen
        // on a very fucked up connection that backs up a lot, then
        // changes level
        if (remoteClient->message.overflowed) {
            SV_DropClient(true);
            remoteClient->message.overflowed = false;
            continue;
        }

        if (remoteClient->message.cursize || remoteClient->dropasap) {
            if (!NET_CanSendMessage(remoteClient->netconnection)) {
                //        I_Printf ("can't write\n");
                continue;
            }

            if (remoteClient->dropasap)      SV_DropClient(false);  // went to another level
            else {
                if (NET_SendMessage(remoteClient->netconnection, &remoteClient->message) == -1)
                    SV_DropClient(true);  // if the message couldn't send, kick off
                SZ_Clear(&remoteClient->message);
                remoteClient->last_message = realtime;
                remoteClient->sendsignon = false;
            }
        }
    }


    // clear muzzle flashes
    SV_CleanupEnts();
}


/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
    ================
    SV_ModelIndex

    ================
*/
int SV_ModelIndex(cString name) {
    if (!name || !name[0]) return 0;

    int i = 0;
    for (; i < MAX_MODELS && sv.model_precache[i]; i++)
        if (!strcmp(sv.model_precache[i], name))
            return i;
    if ((i == MAX_MODELS) ||
        !sv.model_precache[i])
        Sys_Error("SV_ModelIndex: model %s not precached", name);
    return i;
}

/*
    ================
    SV_CreateBaseline

    ================
*/
void SV_CreateBaseline() {

    for (uint32_t entnum = 0; entnum < sv.num_edicts; entnum++) {
        // get the current server version
        edict_p svent = EDICT_NUM(entnum);
        if ((svent->free) ||
            ((entnum > svs.maxClients) &&
                !svent->v.modelindex)
            )
            continue;

        //
        // create entity baseline
        //
        VectorCopy(svent->v.origin, svent->baseline.origin);
        VectorCopy(svent->v.angles, svent->baseline.angles);
        svent->baseline.frame = (int32_t)svent->v.frame;
        svent->baseline.skin = (int32_t)svent->v.skin;
        if ((entnum > 0) && (entnum <= svs.maxClients)) {
            svent->baseline.colormap = (int32_t)entnum;
            svent->baseline.modelindex = SV_ModelIndex("progs/player.mdl");
        }
        else {
            svent->baseline.colormap = 0;
            svent->baseline.modelindex =
                SV_ModelIndex(pr_strings + svent->v.model);
        }

        //
        // add to the message
        //
        MSG_WriteByte(&sv.signon, svc_spawnbaseline);   MSG_WriteShort(&sv.signon, (int16_t)entnum);

        MSG_WriteByte(&sv.signon, (uint8_t)svent->baseline.modelindex);
        MSG_WriteByte(&sv.signon, (uint8_t)svent->baseline.frame);
        MSG_WriteByte(&sv.signon, (uint8_t)svent->baseline.colormap);
        MSG_WriteByte(&sv.signon, (uint8_t)svent->baseline.skin);
        for (int i = 0; i < VECT_DIM; i++) {
            MSG_WriteCoord(&sv.signon, svent->baseline.origin[i]);
            MSG_WriteAngle(&sv.signon, svent->baseline.angles[i]);
        }
    }
}


/*
    ================
    SV_SendReconnect

    Tell all the clients that the server is changing levels
    ================
*/
void SV_SendReconnect() {
    uint8_t data[128];
    sizebuf_t msg = {
        .data = data,
        .cursize = 0,
        .maxsize = sizeof(data),
    };

    MSG_WriteChar(&msg, svc_stufftext);
    MSG_WriteString(&msg, "reconnect\n");
    NET_SendToAll(&msg, 5);

    if (cls.state != ca_dedicated)
#ifdef QUAKE2
        Cbuf_InsertText("reconnect\n");
#else
        Cmd_ExecuteString("reconnect\n", src_command);
#endif
}


/*
    ================
    SV_SaveSpawnparms

    Grabs the current state of each client for saving across the
    transition to another level
    ================
*/
void SV_SaveSpawnparms() {
    svs.serverflags = (uint32_t)pr_global_struct->serverflags;
    remoteClient = svs.clients;
    for (int i = 0; i < svs.maxClients; i++, remoteClient++) {
        if (!remoteClient->active)       continue;

        // call the progs to get default spawn parms for the new client
        pr_global_struct->self = EDICT_TO_PROG(remoteClient->edict);
        PR_ExecuteProgram(pr_global_struct->SetChangeParms);
        for (int j = 0; j < NUM_SPAWN_PARMS; j++)
            remoteClient->spawn_parms[j] = (&pr_global_struct->parm1)[j];
    }
}


/*
    ================
    SV_SpawnServer

    This is called at the start of each level
    ================
*/

void SV_SpawnServer(
    cString server
#ifdef QUAKE2
    , cString startspot
#endif
) {

    // let's not have any servers with no name
    if (hostname.string[0] == 0)
        Cvar_Set("hostname", "UNNAMED");
    scr.centertime_off = 0;

    Con_DPrintf("SpawnServer: %s\n", server);
    svs.changelevel_issued = false;    // now safe to issue another

    //
    // tell all connected clients that we are going to a new level
    //
    if (sv.active) { SV_SendReconnect(); }

    //
    // make cvars consistant
    //
    if (coop.value) Cvar_SetValue("deathmatch", 0);
    current_skill = (int)(skill.value + 0.5);
    CLAMP(0, current_skill, 3);

    Cvar_SetValue("skill", (float)current_skill);

    //
    // set up the new server
    //
    Host_ClearMemory();

    memset(&sv, 0, sizeof(sv));

    strcpy(sv.name, server);
#ifdef QUAKE2
    if (startspot)
        strcpy(sv.startspot, startspot);
#endif

    // load progs to get entity field count
    PR_LoadProgs();

    // allocate server memory
    sv.max_edicts = MAX_EDICTS;

    sv.edicts = Hunk_AllocName((uint32_t)sv.max_edicts * pr_edict_size, "edicts");

    sv.datagram.maxsize = sizeof(sv.datagram_buf);
    sv.datagram.cursize = 0;
    sv.datagram.data = sv.datagram_buf;

    sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
    sv.reliable_datagram.cursize = 0;
    sv.reliable_datagram.data = sv.reliable_datagram_buf;

    sv.signon.maxsize = sizeof(sv.signon_buf);
    sv.signon.cursize = 0;
    sv.signon.data = sv.signon_buf;

    // leave slots at start for clients only
    sv.num_edicts = svs.maxClients + 1;
    for (uint32_t i = 0; i < svs.maxClients; i++) {
        edict_p ent = EDICT_NUM(i + 1);
        svs.clients[i].edict = ent;
    }

    sv.state = ss_loading;
    sv.paused = false;

    sv.time = 1.0;

    strcpy(sv.name, server);
    snprintf(sv.modelname, sizeof(sv.modelname), "maps/%s.bsp", server);
    sv.worldmodel = Mod_ForName(sv.modelname, false);
    if (!sv.worldmodel) {
        Con_Printf("Couldn't spawn server %s\n", sv.modelname);
        sv.active = false;
        return;
    }
    sv.models[1] = sv.worldmodel;

    //
    // clear world interaction links
    //
    SV_ClearWorld();

    sv.sound_precache[0] = pr_strings;

    sv.model_precache[0] = pr_strings;
    sv.model_precache[1] = sv.modelname;
    for (int i = 1; i < sv.worldmodel->numsubmodels; i++) {
        sv.model_precache[1 + i] = _localModels[i];
        sv.models[i + 1] = Mod_ForName(_localModels[i], false);
    }

    //
    // load the rest of the entities
    //
    edict_p ent = EDICT_NUM(0);
    memset(&ent->v, 0, progs->entityfields * 4);
    ent->free = false;
    ent->v.model = sv.worldmodel->name - pr_strings;
    ent->v.modelindex = 1;    // world model
    ent->v.solid = SOLID_BSP;
    ent->v.movetype = MOVETYPE_PUSH;

    if (coop.value) pr_global_struct->coop = coop.value;
    else            pr_global_struct->deathmatch = deathmatch.value;

    pr_global_struct->mapname = sv.name - pr_strings;
#ifdef QUAKE2
    pr_global_struct->startspot = sv.startspot - pr_strings;
#endif

    // serverflags are for cross level information (sigils)
    pr_global_struct->serverflags = (float)svs.serverflags;

    ED_LoadFromFile(sv.worldmodel->entities);

    sv.active = true;

    // all setup is completed, any further precache statements are errors
    sv.state = ss_active;

    // run two frames to allow everything to settle
    host_frametime = 0.1;
    SV_Physics();
    SV_Physics();

    // create a baseline for more efficient communications
    SV_CreateBaseline();

    // send serverinfo to all connected clients
    remoteClient = svs.clients;
    for (int i = 0; i < svs.maxClients; i++, remoteClient++)
        if (remoteClient->active)
            SV_SendServerinfo(remoteClient);

    Con_DPrintf("Server spawned.\n");
}

