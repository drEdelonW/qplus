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
// client.h
#include "mathlib.h"
#include "enginedefs.h"
#include "platformdefs.h"
#include "sizebuf.h"
#include "screen.h"
#include "vid.h"
#include "bspfile.h"
#include "cvar_q1.h"

typedef struct {
	vec3_t	viewangles;

	// intended velocities
	float	forwardmove;
	float	sidemove;
	float	upmove;
#ifdef QUAKE2
	uint8_t	lightlevel;
#endif
} usercmd_t;

typedef struct {
	int32_t		length;
	char	map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct {
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int32_t		frags;
	int32_t		colors;			// two 4 bit fields
	uint8_t	translations[VID_GRADES * 256];
} scoreboard_t;

typedef struct {
	int32_t		destcolor[3];
	int32_t		percent;		// 0-256
} cshift_t;

#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define	NUM_CSHIFTS		4

#define	NAME_LENGTH	64


//
// client_state_t should hold all pieces of the client state
//

#define	SIGNONS		4			// signon messages to receive before connected

#define	MAX_DLIGHTS		32
typedef struct {
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int32_t		key;
#ifdef QUAKE2
	bool	dark;			// subtracts light instead of adding
#endif
} dlight_t;


#define	MAX_BEAMS	24
typedef struct {
	int32_t		entity;
	struct model_s* model;
	float	endtime;
	vec3_t	start, end;
} beam_t;

#define	MAX_EFRAGS		640

#define	MAX_MAPSTRING	2048
#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

typedef enum {
	ca_dedicated, 		// a dedicated server with no ability to start a client
	ca_disconnected, 	// full screen console with no connection
	ca_connected		// valid netcon, talking to a server
} cactive_t;

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct {
	cactive_t	state;

	// personalization data sent to server
	char		mapstring[MAX_QPATH];
	char		spawnparms[MAX_MAPSTRING];	// to restart a level

	// demo loop control
	int32_t			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

	// demo recording info must be here, because record is started before
	// entering a map (and clearing client_state_t)
	bool	demorecording;
	bool	demoplayback;
	bool	timedemo;
	int32_t			forcetrack;			// -1 = use normal cd track
	FILE* demofile;
	int32_t			td_lastframe;		// to meter out one message a frame
	int32_t			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo


	// connection information
	int32_t			signon;			// 0 to SIGNONS
	struct qsocket_s* netcon;
	sizebuf_t	message;		// writing buffer to send to server

} client_static_t;

extern client_static_t	cls;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct {
	int32_t			movemessages;	// since connecting to this server
	// throw out the first couple, so the player
	// doesn't accidentally do something the
	// first frame
	usercmd_t	cmd;			// last command sent to the server

	// information for local display
	int32_t			stats[MAX_CL_STATS];	// health, etc
	int32_t			items;			// inventory bit flags
	float		item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  The server sets punchangle when
	// the view is temporarliy offset, and an angle reset commands at the start
	// of each level and after teleporting.
	vec3_t		mviewangles[2];	// during demo playback viewangles is lerped
	// between these
	vec3_t		viewangles;

	vec3_t		mvelocity[2];	// update by server, used for lean+bob
	// (0 is newest)
	vec3_t		velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset

	// pitch drifting vars
	float		idealpitch;
	float		pitchvel;
	bool	nodrift;
	float		driftmove;
	double		laststop;

	float		viewheight;
	float		crouch;			// local amount for smoothing stepups

	bool	paused;			// send over by server
	bool	onground;
	bool	inwater;

	int32_t			intermission;	// don't change view angle, full screen, etc
	int32_t			completed_time;	// latched at intermission start

	double		mtime[2];		// the timestamp of last two messages
	double		time;			// clients view of time, should be between
	// servertime and oldservertime to generate
	// a lerp point for other data
	double		oldtime;		// previous cl.time, time-oldtime is used
	// to decay light values and smooth step ups


	float		last_received_message;	// (realtime) for net trouble icon

	//
	// information that is static for the entire time connected to a server
	//
	struct model_s* model_precache[MAX_MODELS];
	struct sfx_s* sound_precache[MAX_SOUNDS];

	char		levelname[40];	// for display on solo scoreboard
	int32_t			viewentity;		// cl_entitites[cl.viewentity] = player
	int32_t			maxclients;
	int32_t			gametype;

	// refresh related state
	struct model_s* worldmodel;	// cl_entitites[0].model
	struct efrag_s* free_efrags;
	int32_t			num_entities;	// held in cl_entities array
	int32_t			num_statics;	// held in cl_staticentities array
	entity_t	viewent;			// the gun model

	int32_t			cdtrack, looptrack;	// cd audio

	// frag scoreboard
	scoreboard_t* scores;		// [cl.maxclients]

#ifdef QUAKE2
	// light level at player's position including dlights
	// this is sent back to the server each frame
	// architectually ugly but it works
	int32_t			light_level;
#endif
} client_state_t;




#define	MAX_TEMP_ENTITIES	64			// lightning bolts, etc
#define	MAX_STATIC_ENTITIES	128			// torches, etc

extern	client_state_t	cl;

// FIXME, allocate dynamically
// extern	efrag_t			cl_efrags[MAX_EFRAGS];
extern	entity_t        cl_entities[MAX_EDICTS];
extern	entity_t        cl_static_entities[MAX_STATIC_ENTITIES];
extern	lightstyle_t    cl_lightstyle[MAX_LIGHTSTYLES];
extern	dlight_t        cl_dlights[MAX_DLIGHTS];
extern	entity_t        cl_temp_entities[MAX_TEMP_ENTITIES];
extern	beam_t          cl_beams[MAX_BEAMS];

//=============================================================================

//
// cl_main
//
dlight_t* CL_AllocDlight(int32_t key);
void	CL_DecayLights();

void CL_Init();

void CL_EstablishConnection(cstring host);
void CL_Signon1();
void CL_Signon2();
void CL_Signon3();
void CL_Signon4();

void CL_Disconnect();
void CL_Disconnect_f();
void CL_NextDemo();

#define			MAX_VISEDICTS	256
extern	int32_t				cl_numvisedicts;
extern	entity_t* cl_visedicts[MAX_VISEDICTS];

//
// cl_input
//
typedef struct {
	int32_t down[2];    // key nums holding it down
	int32_t state;      // low bit is down state
} kbutton_t;

extern  kbutton_t   in_mlook, in_klook;
extern  kbutton_t   in_strafe;
extern  kbutton_t   in_speed;

void CL_InitInput();
void CL_SendCmd();
void CL_SendMove(usercmd_t* cmd);

void CL_ParseTEnt();
void CL_UpdateTEnts();

void CL_ClearState();


int32_t  CL_ReadFromServer();
void CL_WriteToServer(usercmd_t* cmd);
void CL_BaseMove(usercmd_t* cmd);


float CL_KeyState(kbutton_t* key);
cstring Key_KeynumToString(int32_t keynum);

//
// cl_demo.c
//
void CL_StopPlayback();
int32_t CL_GetMessage();

void CL_Stop_f();
void CL_Record_f();
void CL_PlayDemo_f();
void CL_TimeDemo_f();

//
// cl_parse.c
//
void CL_ParseServerMessage();
void CL_NewTranslation(int32_t slot);

//
// view
//
void V_StartPitchDrift();
void V_StopPitchDrift();

void V_RenderView();
void V_UpdatePalette();
void V_Register();
void V_ParseDamage();
void V_SetContentsColor(contents_t contents);


//
// cl_tent
//
void CL_InitTEnts();
void CL_SignonReply();
