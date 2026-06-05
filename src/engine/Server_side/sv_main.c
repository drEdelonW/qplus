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

#include "server_priv.h"
#include "sv_net.h"
#include <string.h>
#include "screen.h"
#include "q_tools.h"
#include "cvar_q1.h"
#include "console.h"
#include "host.h"
#include "progs.h"
#include "GlobVars.h"

server_t    sv;
sv_static_t svs;
RmtClient_p remoteClient;   // current client   TODO: make it thread safety!!!
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

cString SV_GetName() {
    return sv.name;
}

bool SV_IsActive() {
    return sv.active;
}

LegacyTimeStamp_t SV_GetTime() {
    return sv.time;
}

void SV_SetTime(LegacyTimeStamp_t time) {
    sv.time = time;
}


/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

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
    client->edict = ED_GetEDictByIdx(clientnum + 1);
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
        if (i == svs.maxClients)        Host_SysError("Host_CheckForNewClients: no free clients");

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

//=============================================================================


/*
    =============
    SV_CleanupEnts

    =============
*/
void SV_CleanupEnts() {
    edict_p ent = ED_GetEDictFirst();
    for (int e = 1; e < EdictsNum; e++, ent = ED_GetEDictNext(ent)) {
        ent->v.effects = (int)ent->v.effects & ~EF_MUZZLEFLASH;
    }
}


/*
    =======================
    SV_SendClientMessages
    =======================
*/
void SV_SendClientMessages() {
    SV_UpdateToReliableMessages();  // update frags, names, etc

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
        Host_SysError("SV_ModelIndex: model %s not precached", name);
    return i;
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
        pr_global_struct->self = ED_GetEDictOffs(remoteClient->edict);
        PR_ExecuteProgram(pr_global_struct->SetChangeParms);
        for (int j = 0; j < NUM_SPAWN_PARMS; j++)
            remoteClient->spawn_parms[j] = (&pr_global_struct->parm1)[j];
    }
}

#include "z_hunk.h"
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
    if (SV_IsActive()) { SV_SendReconnect(); }

    //
    // make cvars consistant
    //
    if (coop.value) Cvar_SetValue("deathmatch", 0);
    current_skill = (int)(skill.value + 0.5f);
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
    // WARNING!!! don't use [EdictSize] before PR_LoadProgs() called!!!
    if (!EdictSize) Host_Error("EdictSize - not inited\n");
    Edicts = Hunk_AllocName((uint32_t)EdictsMax * EdictSize, "edicts");
    // sv.edicts = Edicts;

    // leave slots at start for clients only
    EdictsNum = svs.maxClients + 1;
    for (uint32_t i = 0; i < svs.maxClients; i++) {
#if 0
        edict_p ent = ED_GetEDictByIdx(i + 1);
        svs.clients[i].edict = ent;
#else
        svs.clients[i].edict = ED_GetEDictByIdx(i + 1);
#endif
    }

#if 0
    sv.datagram.maxsize = sizeof(sv.datagram_buf);
    sv.datagram.cursize = 0;
    sv.datagram.data = sv.datagram_buf;

    sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
    sv.reliable_datagram.cursize = 0;
    sv.reliable_datagram.data = sv.reliable_datagram_buf;

    sv.signon.maxsize = sizeof(sv.signon_buf);
    sv.signon.cursize = 0;
    sv.signon.data = sv.signon_buf;
#else
    sv.datagram = (sizebuf_t){
        .maxsize = sizeof(sv.datagram_buf),
        .cursize = 0,
        .data = sv.datagram_buf
    };
    sv.reliable_datagram = (sizebuf_t){
        .maxsize = sizeof(sv.reliable_datagram_buf),
        .cursize = 0,
        .data = sv.reliable_datagram_buf
    };
    sv.signon = (sizebuf_t){
        .maxsize = sizeof(sv.signon_buf),
        .cursize = 0,
        .data = sv.signon_buf
    };
#endif

    sv.state = ss_loading;
    sv.paused = false;

    SV_SetTime(1.0);

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

    // sv.sound_precache[0] = pr_strings;

    sv.model_precache[0] = PR_GetQString(0); // not sure
    sv.model_precache[1] = sv.modelname;
    for (int i = 1; i < sv.worldmodel->numSubModels; i++) {
        sv.model_precache[1 + i] = _localModels[i];
        sv.models[i + 1] = Mod_ForName(_localModels[i], false);
    }

    //
    // load the rest of the entities
    //
    edict_p ent = ED_GetEDictByIdx(0);
    memset(&ent->v, 0, progs->entityfields * 4);
    ent->free = false;
    ent->v.model = PR_SetQString(sv.worldmodel->name);
    ent->v.modelindex = 1;    // world model
    ent->v.solid = SOLID_BSP;
    ent->v.movetype = MOVETYPE_PUSH;

    if (coop.value) pr_global_struct->coop = coop.value;
    else            pr_global_struct->deathmatch = deathmatch.value;

    pr_global_struct->mapname = PR_SetQString(SV_GetName());
#ifdef QUAKE2
    pr_global_struct->startspot = PR_SetQString(sv.startspot);
#endif

    // serverflags are for cross level information (sigils)
    pr_global_struct->serverflags = (float)svs.serverflags;

    ED_LoadFromFile(sv.worldmodel->entities);

    sv.active = true;

    sv.state = ss_active;    // all setup is completed, any further precache statements are errors

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

