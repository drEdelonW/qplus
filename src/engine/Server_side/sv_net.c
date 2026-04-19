#include "server_priv.h"
#include "sv_net.h"
#include "msg.h"
#include "protocol.h"
#include "versions.h"
#include "progs.h"
#include "q_tools.h"
#include <string.h>
#include "console.h"
#include "host.h"
#include "sound.h"
#include "gamedefs.h"
#include "cmd.h"

#include "cvar_q1.h"


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
    if ((volume < 0) || (volume > 255))         Host_SysError("SV_StartSound: volume = %i", volume);
    if ((attenuation < 0) || (attenuation > 4)) Host_SysError("SV_StartSound: attenuation = %f", attenuation);
    if ((channel < 0) || (channel > 7))         Host_SysError("SV_StartSound: channel = %i", channel);

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

    channel |= (int)(ED_GetEDictIdx(entity) << 3);

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

    MSG_WriteByte(pBuf, svc_serverinfo); MSG_WriteLong(pBuf, PROTOCOL_VERSION); MSG_WriteByte(pBuf, svs.maxClients);

    MSG_WriteByte(pBuf, (!coop.value && deathmatch.value) ? GAME_DEATHMATCH : GAME_COOP);

    snprintf(message, sizeof(message), "%s", PR_GetQString(Edicts->v.message));

    MSG_WriteString(pBuf, message);

    for (cStringArray s = (sv.model_precache + 1); *s; s++)
        MSG_WriteString(pBuf, *s);
    MSG_WriteByte(pBuf, 0);

    for (cStringArray s = (sv.sound_precache + 1); *s; s++)
        MSG_WriteString(pBuf, *s);
    MSG_WriteByte(pBuf, 0);

    // send music
    MSG_WriteByte(pBuf, svc_cdtrack);   MSG_WriteByte(pBuf, (uint8_t)Edicts->v.sounds); MSG_WriteByte(pBuf, (uint8_t)Edicts->v.sounds);

    // set view
    MSG_WriteByte(pBuf, svc_setview);   MSG_WriteShort(pBuf, (int16_t)ED_GetEDictIdx(client->edict));

    MSG_WriteByte(pBuf, svc_signonnum); MSG_WriteByte(pBuf, 1);

    client->sendsignon = true;
    client->spawned = false;    // need prespawn, spawn, etc
}



/*
    =============
    SV_WriteEntitiesToClient

    =============
*/
void SV_WriteEntitiesToClient(edict_p clent, sizebuf_p msg) {
    // find the client's PVS
    vec3_t  org;   VectorAdd(clent->v.origin, clent->v.view_ofs, org);
    uint8_p pvs = SV_FatPVS(org);

    // send over all entities (excpet the client) that touch the pvs
    edict_p ent = ED_GetEDictFirst();
    for (int e = 1; e < sv.num_edicts; e++, ent = ED_GetEDictNext(ent)) {
#ifdef QUAKE2
        // don't send if flagged for NODRAW and there are no lighting effects
        if (ent->v.effects == EF_NODRAW)
            continue;
#endif

        // ignore if not touching a PV leaf
        if (ent != clent) {  // clent is ALLWAYS sent
            // ignore ents without visible models
            // if (!ent->v.modelindex || !pr_strings[ent->v.model])    continue;
            if (!ent->v.modelindex || !PR_GetQString(ent->v.model)[0])    continue;

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
    ==================
    SV_WriteClientdataToMessage

    ==================
*/
void SV_WriteClientdataToMessage(edict_p ent, sizebuf_p msg) {
    //
    // send a damage message
    //
    if (ent->v.dmg_take || ent->v.dmg_save) {
        edict_p other = ED_GetEDictByOffs(ent->v.dmg_inflictor);
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

    MSG_WriteByte(msg, svc_clientdata); MSG_WriteShort(msg, (int16_t)bits);
    if (bits & SU_VIEWHEIGHT)           MSG_WriteChar(msg, (int8_t)ent->v.view_ofs[2]);
    if (bits & SU_IDEALPITCH)           MSG_WriteChar(msg, (int8_t)ent->v.idealpitch);
    for (int i = 0; i < VECT_DIM; i++) {
        if (bits & (SU_PUNCH1 << i))    MSG_WriteChar(msg, (int8_t)ent->v.punchangle[i]);
        if (bits & (SU_VELOCITY1 << i)) MSG_WriteChar(msg, (int8_t)ent->v.velocity[i] / 16);
    }

    // [always sent]
    /* if (bits & SU_ITEMS) */          MSG_WriteLong(msg, items);

    if (bits & SU_WEAPONFRAME)          MSG_WriteByte(msg, (uint8_t)ent->v.weaponframe);
    if (bits & SU_ARMOR)                MSG_WriteByte(msg, (uint8_t)ent->v.armorvalue);
    if (bits & SU_WEAPON)               MSG_WriteByte(msg, (uint8_t)SV_ModelIndex(PR_GetQString(ent->v.weaponmodel)));

    MSG_WriteShort(msg, (int16_t)ent->v.health);
    MSG_WriteByte(msg, (uint8_t)ent->v.currentammo);
    MSG_WriteByte(msg, (uint8_t)ent->v.ammo_shells);
    MSG_WriteByte(msg, (uint8_t)ent->v.ammo_nails);
    MSG_WriteByte(msg, (uint8_t)ent->v.ammo_rockets);
    MSG_WriteByte(msg, (uint8_t)ent->v.ammo_cells);

    if (standard_quake)                 MSG_WriteByte(msg, (uint8_t)ent->v.weapon);
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
bool SV_SendClientDatagram(RmtClient_p client) {
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
    ================
    SV_CreateBaseline

    ================
*/
void SV_CreateBaseline() {

    for (uint32_t entnum = 0; entnum < sv.num_edicts; entnum++) {
        // get the current server version
        edict_p svent = ED_GetEDictByIdx(entnum);
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
                SV_ModelIndex(PR_GetQString(svent->v.model));
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

#undef SERVER   // TODO: remove this workaround
#include "client.h"

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

    if (!Host_IsDedicated())
#ifdef QUAKE2
        Cbuf_InsertText("reconnect\n");
#else
        Cmd_ExecuteString("reconnect\n", src_command);
#endif
}
