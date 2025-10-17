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

#include <setjmp.h>
#include "client.h"
#include "model.h"
#include "progs.h"
#include "sizebuf.h"
#include "net.h"

struct client_s;
typedef struct client_s client_t;
typedef client_t* client_p;

typedef struct {
	int32_t     maxclients;
	int32_t     maxclientslimit;
	client_p    clients;            // [maxclients]
	int32_t     serverflags;        // episode completion information
	bool        changelevel_issued; // cleared when at SV_SpawnServer
} sv_static_t;

//=============================================================================

typedef enum {
	ss_loading,
	ss_active
} sv_state_t;

typedef struct {
	bool    active;         // false if only a net client
	bool    paused;
	bool    loadgame;       // handle connections specially
	double  time;
	int32_t lastcheck;      // used by PF_checkclient
	double  lastchecktime;
	char    name[64];       // map name
#ifdef QUAKE2
	char    startspot[64];
#endif
	char        modelname[64];  // maps/<name>.bsp, for model_precache[0]
	Model_p     worldmodel;
	cString     model_precache[MAX_MODELS];	    // NULL terminated
	Model_p     models[MAX_MODELS];
	cString     sound_precache[MAX_SOUNDS];	    // NULL terminated
	cString     lightstyles[MAX_LIGHTSTYLES];
	int32_t     num_edicts;
	int32_t     max_edicts;
	edict_p     edicts;         // can NOT be array indexed, because edict_t is variable sized, but can be used to reference the world ent
	sv_state_t state; // some actions are only valid during load
	sizebuf_t   datagram;
	uint8_t     datagram_buf[MAX_DATAGRAM];
	sizebuf_t   reliable_datagram;	// copied to all clients at end of frame
	uint8_t     reliable_datagram_buf[MAX_DATAGRAM];
	sizebuf_t   signon;
	uint8_t     signon_buf[8192];
} server_t;


#define	NUM_PING_TIMES		16
#define	NUM_SPAWN_PARMS		16

typedef struct client_s {
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
    edict_p     edict;      // EDICT_NUM(clientnum+1)
    char        name[32];   // for printing to other people
    int32_t     colors;
    float       ping_times[NUM_PING_TIMES];
    int32_t     num_pings;  // ping_times[num_pings%NUM_PING_TIMES]
    float       spawn_parms[NUM_SPAWN_PARMS]; // spawn parms are carried from level to level
    int32_t     old_frags;  // client known data for deltas
} client_t;


//=============================================================================

// edict->movetype values
typedef enum {
	MOVETYPE_NONE           = 0,  // never moves
	MOVETYPE_ANGLENOCLIP    = 1,
	MOVETYPE_ANGLECLIP      = 2,
	MOVETYPE_WALK           = 3,  // gravity
	MOVETYPE_STEP           = 4,  // gravity, special edge handling
	MOVETYPE_FLY            = 5,
	MOVETYPE_TOSS           = 6,  // gravity
	MOVETYPE_PUSH           = 7,  // no clip to world, push and crush
	MOVETYPE_NOCLIP         = 8,
	MOVETYPE_FLYMISSILE     = 9,  // extra size to monsters
	MOVETYPE_BOUNCE         = 10,
#ifdef QUAKE2
	MOVETYPE_BOUNCEMISSILE = 11, // bounce w/o gravity
	MOVETYPE_FOLLOW = 12, // track movement of aiment
#endif
} movetype_t;

// edict->solid values
typedef enum {
    SOLID_NOT      = 0, // no interaction with other objects
    SOLID_TRIGGER  = 1, // touch on edge, but not blocking
    SOLID_BBOX     = 2, // touch on edge, block
    SOLID_SLIDEBOX = 3, // touch on edge, but not an onground
    SOLID_BSP      = 4  // BSP clip, touch on edge, block
} solid_t;

// edict->deadflag values
typedef enum {
    DEAD_NO    = 0, // alive
    DEAD_DYING = 1, // in the process of dying
    DEAD_DEAD  = 2  // fully dead
} deadflag_t;

typedef enum {
    DAMAGE_NO  = 0, // does not take damage
    DAMAGE_YES = 1, // always takes damage
    DAMAGE_AIM = 2  // takes damage only with aim
} damage_t;

// edict->flags
typedef enum {
    FL_FLY            = 1 << 0,   // 0000...0001
    FL_SWIM           = 1 << 1,   // 0000...0010
    // FL_GLIMPSE        = 1 << 1,
    FL_CONVEYOR       = 1 << 2,   // 0000...0100
    FL_CLIENT         = 1 << 3,   // 0000...1000
    FL_INWATER        = 1 << 4,   // 0001...0000
    FL_MONSTER        = 1 << 5,   // 0010...0000
    FL_GODMODE        = 1 << 6,
    FL_NOTARGET       = 1 << 7,
    FL_ITEM           = 1 << 8,
    FL_ONGROUND       = 1 << 9,
    FL_PARTIALGROUND  = 1 << 10,  // not all corners are valid
    FL_WATERJUMP      = 1 << 11,  // player jumping out of water
    FL_JUMPRELEASED   = 1 << 12,  // for jump debouncing
#ifdef QUAKE2
    FL_FLASHLIGHT     = 1 << 13,
    FL_ARCHIVE_OVERRIDE = 1 << 20
#endif
} EntityFlags_t;



typedef enum {
    SPAWNFLAG_NOT_EASY       = 1 << 8,  // 0x0100
    SPAWNFLAG_NOT_MEDIUM     = 1 << 9,  // 0x0200
    SPAWNFLAG_NOT_HARD       = 1 << 10, // 0x0400
    SPAWNFLAG_NOT_DEATHMATCH = 1 << 11  // 0x0800
} SpawnFlags_t;

#ifdef QUAKE2
// server flags
typedef enum {
    SFL_EPISODE_1      = 1 << 0,   // 0x0001
    SFL_EPISODE_2      = 1 << 1,   // 0x0002
    SFL_EPISODE_3      = 1 << 2,   // 0x0004
    SFL_EPISODE_4      = 1 << 3,   // 0x0008
    SFL_NEW_UNIT       = 1 << 4,   // 0x0010
    SFL_NEW_EPISODE    = 1 << 5,   // 0x0020
    SFL_CROSS_TRIGGERS = 0xFF00    // 65280, covers multiple bits
} spawnlevel_flags_t;
#endif

//============================================================================

extern sv_static_t  svs; // persistant server info
extern server_t     sv;         // local server
extern client_p     host_client;
extern jmp_buf      host_abortserver;
extern double       host_time;
extern edict_p      sv_player;

//===========================================================

void SV_Init();

void SV_StartParticle(vec3_t org, vec3_t dir, int color, int32_t count);
void SV_StartSound(edict_p entity, int channel, cString sample, int volume,	float attenuation);

void SV_DropClient(bool crash);

void SV_SendClientMessages();
void SV_ClearDatagram();

int SV_ModelIndex(cString name);

void SV_SetIdealPitch();

void SV_AddUpdates();

void SV_ClientThink();
void SV_AddClientToServer(qsocket_p ret);

void SV_ClientPrintf(cString fmt, ...);
void SV_BroadcastPrintf(cString fmt, ...);

void SV_Physics();

bool SV_CheckBottom(edict_p ent);
bool SV_movestep(edict_p ent, vec3_t move, bool relink);

void SV_WriteClientdataToMessage(edict_p ent, sizebuf_p msg);

void SV_MoveToGoal();

void SV_CheckForNewClients();
void SV_RunClients();
void SV_SaveSpawnparms();
#ifdef QUAKE2
void SV_SpawnServer(cString server, cString startspot);
#else
void SV_SpawnServer(cString server);
#endif
