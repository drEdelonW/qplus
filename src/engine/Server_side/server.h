#pragma once
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
// server.h
#define SERVER
#ifdef CLIENT
#   error CLIENT defined
#endif
#include "model/model.h"
#include "sizebuf.h"
#include "net.h"
#include "UserCmd.h"
#include "Edict.h"


//=============================================================================

typedef enum {
    ss_loading,
    ss_active
} sv_state_e;

typedef struct {
    bool        active;         // false if only a net client
    bool        paused;
    bool        loadgame;       // handle connections specially
    double      time;
    uint8_t     lastcheck;      // used by PF_checkclient
    double      lastchecktime;
    char        name[NAME_LENGTH];       // map name
#ifdef QUAKE2
    char        startspot[NAME_LENGTH];
#endif
    char        modelname[NAME_LENGTH];  // maps/<name>.bsp, for model_precache[0]
    Model_p     worldmodel;
    cString     model_precache[MAX_MODELS];	    // NULL terminated
    Model_p     models[MAX_MODELS];

    cString     sound_precache[MAX_SOUNDS];	    // NULL terminated
    cString     lightstyles[MAX_LIGHTSTYLES];

    int32_t     num_edicts;
    int32_t     max_edicts;
    // edict_p     edicts;         // can NOT be array indexed, because edict_t is variable sized, but can be used to reference the world ent

    sv_state_e  state;          // some actions are only valid during load
    sizebuf_t   datagram;
    uint8_t     datagram_buf[MAX_DATAGRAM];
    sizebuf_t   reliable_datagram;	// copied to all clients at end of frame
    uint8_t     reliable_datagram_buf[MAX_DATAGRAM];
    sizebuf_t   signon;
    uint8_t     signon_buf[8192];
} server_t;



#define	NUM_PING_TIMES		16
#define	NUM_SPAWN_PARMS		16

typedef struct {
    bool        active;     // false = client is free
    bool        spawned;    // false = don't send datagrams
    bool        dropasap;   // has been told to go to another level
    bool        privileged; // can execute any host command
    bool        sendsignon; // only valid before spawned
    double      last_message;   // reliable messages must be sent periodically
    qsocket_p   netconnection;  // communications handle
    UserCmd_t   cmd;        // movement
    vec3_t      wishdir;    // intended motion calced from cmd
    sizebuf_t   message;    // can be added to at any time, copied and clear once per frame
    uint8_t     msgbuf[MAX_MSGLEN];
    edict_p     edict;      // ED_GetEDictByIdx(clientnum+1)
    char        name[32];   // for printing to other people
    uint8_t     colors;
    float       ping_times[NUM_PING_TIMES];
    int32_t     num_pings;  // ping_times[num_pings%NUM_PING_TIMES]
    float       spawn_parms[NUM_SPAWN_PARMS]; // spawn parms are carried from level to level
    int16_t     old_frags;  // client known data for deltas
} RmtClient_t;
typedef RmtClient_t* RmtClient_p;


//=============================================================================

typedef struct {
    uint8_t     maxClients;
    uint8_t     maxClientsLimit;
    RmtClient_p clients;            // [maxClients]
    uint32_t    serverflags;        // episode completion information
    bool        changelevel_issued; // cleared when at SV_SpawnServer
} sv_static_t;


//============================================================================

extern sv_static_t  svs;    // persistent server info
extern server_t     sv;     // local server
extern RmtClient_p  remoteClient;
extern edict_p      sv_player;

//===========================================================
#ifdef __cplusplus
extern "C" {
#endif
    void SV_Init();
    cString SV_GetName();
    bool SV_IsActive();

    void SV_StartSound(edict_p entity, int channel, cString sample, int volume, float attenuation);

    void SV_DropClient(bool crash);

    void SV_SendClientMessages();
    void SV_ClearDatagram();

    void SV_ClientThink();

    void SV_ClientPrintf(cStringRO fmt, ...);
    void SV_BroadcastPrintf(cString fmt, ...);

    void SV_Physics();

    void SV_WriteClientdataToMessage(edict_p ent, sizebuf_p msg);

    void SV_CheckForNewClients();
    void SV_RunClients();
    void SV_SaveSpawnparms();
    void SV_SpawnServer(
        cString server
#   ifdef QUAKE2
        , cString startspot
#   endif
    );

// called after the world model has been loaded, before linking any entities
#ifdef __cplusplus
}
#endif