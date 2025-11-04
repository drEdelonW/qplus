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
#include <stdio.h>
#include "enginedefs.h"
#include "platformdefs.h"
#include "bspfile.h"
#include "render.h"
#include "sound/sound_struct.h"
#include "net.h"



typedef struct {
    vec3_t viewangles;
    // intended velocities
    float forwardmove;
    float sidemove;
    float upmove;
#ifdef QUAKE2
    uint8_t lightlevel;
#endif
} UserCmd_t;
typedef UserCmd_t* UserCmd_p;

typedef struct {
    int32_t length;
    char    map[MAX_STYLESTRING];
} LightStyle_t;

typedef struct {
    char    name[MAX_SCOREBOARDNAME];
    float   entertime;
    int32_t frags;
    int32_t colors;   // two 4 bit fields
    uint8_t translations[VID_GRADES * 256];
} ScoreBoard_t;
typedef ScoreBoard_t* ScoreBoard_p;

typedef struct {
    uint8_t  destcolor[3];
    uint8_t  percent;  // 0-256
} ColorShift_t;

typedef enum cshift_kind_e {
    CSHIFT_CONTENTS = 0,
    CSHIFT_DAMAGE   = 1,
    CSHIFT_BONUS    = 2,
    CSHIFT_POWERUP  = 3,

    NUM_CSHIFTS // should be last
} cshift_kind_t;

#define NAME_LENGTH 64


//
// ClientState_t should hold all pieces of the client state
//

#define SIGNONS  4   // signon messages to receive before connected

#define MAX_DLIGHTS  32
typedef struct {
    vec3_t  origin;
    float   radius;
    float   die;    // stop lighting after this time
    float   decay;    // drop this each second
    float   minlight;   // don't add when contributing less
    int32_t key;
#ifdef QUAKE2
    bool dark;   // subtracts light instead of adding
#endif
} dLight_t;
typedef dLight_t* dLight_p;


#define MAX_BEAMS 24
typedef struct {
    int32_t entity;
    Model_p model;
    float   endtime;
    vec3_t  start, end;
} Beam_t;
typedef Beam_t* Beam_p;

#define MAX_EFRAGS  640

#define MAX_MAPSTRING 2048
#define MAX_DEMOS  8
#define MAX_DEMONAME 16

typedef enum {
    ca_dedicated,   // a dedicated server with no ability to start a client
    ca_disconnected,  // full screen console with no connection
    ca_connected  // valid netcon, talking to a server
} ClientStatus;

//
// the ClientStatic_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct {
    ClientStatus state;

    // personalization data sent to server
    char    mapstring[MAX_QPATH];
    char    spawnparms[MAX_MAPSTRING]; // to restart a level

    // demo loop control
    int32_t demonum;  // -1 = don't play demos
    char    demos[MAX_DEMOS][MAX_DEMONAME];  // when not playing

    // demo recording info must be here, because record is started before
    // entering a map (and clearing ClientState_t)
    bool    demorecording;
    bool    demoplayback;
    bool    timedemo;
    int32_t forcetrack;   // -1 = use normal cd track
    FILE* demofile;
    int32_t td_lastframe;  // to meter out one message a frame
    int32_t td_startframe;  // host_framecount at start
    float   td_starttime;  // realtime at second frame of timedemo

    // connection information
    int32_t     signon;   // 0 to SIGNONS
    qsocket_p   netcon;
    sizebuf_t   message;  // writing buffer to send to server

} ClientStatic_t;

extern ClientStatic_t cls;



//
// stats are integers communicated to the client by the server
//
typedef enum {
    STAT_HEALTH        = 0,
    STAT_FRAGS         = 1,
    STAT_WEAPON        = 2,
    STAT_AMMO          = 3,
    STAT_ARMOR         = 4,
    STAT_WEAPONFRAME   = 5,
    STAT_SHELLS        = 6,
    STAT_NAILS         = 7,
    STAT_ROCKETS       = 8,
    STAT_CELLS         = 9,
    STAT_ACTIVEWEAPON  = 10,
    STAT_TOTALSECRETS  = 11,
    STAT_TOTALMONSTERS = 12,
    STAT_SECRETS       = 13, // bumped client-side by svc_foundsecret
    STAT_MONSTERS      = 14,  // bumped by svc_killedmonster

    MAX_CL_STATS       = 32
} stat_t;

//
// the ClientState_t structure is wiped completely at every
// server signon
//
typedef struct {
    int32_t   movemessages; // since connecting to this server
    // throw out the first couple, so the player
    // doesn't accidentally do something the
    // first frame
    UserCmd_t   cmd;   // last command sent to the server

    // information for local display
    int32_t stats[MAX_CL_STATS]; // health, etc
    int32_t items;   // inventory bit flags
    float   item_gettime[32]; // cl.time of aquiring item, for blinking
    float   faceanimtime; // use anim frame if cl.time < this

    ColorShift_t    cshifts[NUM_CSHIFTS]; // color shifts for damage, powerups
    ColorShift_t    prev_cshifts[NUM_CSHIFTS]; // and content types

    // the client maintains its own idea of view angles, which are
    // sent to the server each frame.  The server sets punchangle when
    // the view is temporarliy offset, and an angle reset commands at the start
    // of each level and after teleporting.
    vec3_t  mviewangles[2]; // during demo playback viewangles is lerped between these
    vec3_t  viewangles;
    vec3_t  mvelocity[2]; // update by server, used for lean+bob (0 is newest)
    vec3_t  velocity;  // lerped between mvelocity[0] and [1]
    vec3_t  punchangle;  // temporary offset

    // pitch drifting vars
    float   idealpitch;
    float   pitchvel;
    bool    nodrift;
    float   driftmove;
    double  laststop;

    float   viewheight;
    float   crouch;   // local amount for smoothing stepups

    bool    paused;   // send over by server
    bool    onground;
    bool    inwater;

    int32_t intermission; // don't change view angle, full screen, etc
    int32_t completed_time; // latched at intermission start

    double  mtime[2];  // the timestamp of last two messages
    double  time;   // clients view of time, should be between  servertime and oldservertime to generate  a lerp point for other data
    double  oldtime;  // previous cl.time, time-oldtime is used  to decay light values and smooth step ups

    float  last_received_message; // (realtime) for net trouble icon

    //
    // information that is static for the entire time connected to a server
    //
    Model_p model_precache[MAX_MODELS];
    sfx_p   sound_precache[MAX_SOUNDS];

    char    levelname[40]; // for display on solo scoreboard
    int32_t viewentity;  // cl_entitites[cl.viewentity] = player
    int32_t maxclients;
    int32_t gametype;

    // refresh related state
    Model_p worldmodel; // cl_entitites[0].model
    efrag_p free_efrags;
    int32_t num_entities; // held in cl_entities array
    int32_t num_statics; // held in cl_staticentities array
    r_Entity_t viewent;   // the gun model
    int32_t cdtrack, looptrack; // cd audio

    // frag scoreboard
    ScoreBoard_p scores;  // [cl.maxclients]
#ifdef QUAKE2
    // light level at player's position including dlights
    // this is sent back to the server each frame
    // architectually ugly but it works
    int32_t   light_level;
#endif
} ClientState_t;

#define MAX_TEMP_ENTITIES 64   // lightning bolts, etc
#define MAX_STATIC_ENTITIES 128   // torches, etc

extern ClientState_t cl;

// FIXME, allocate dynamically
extern r_Entity_t   cl_entities[MAX_EDICTS];
extern r_Entity_t   cl_static_entities[MAX_STATIC_ENTITIES];
extern LightStyle_t cl_lightstyle[MAX_LIGHTSTYLES];
extern dLight_t     cl_dlights[MAX_DLIGHTS];
extern r_Entity_t   cl_temp_entities[MAX_TEMP_ENTITIES];
extern Beam_t       cl_beams[MAX_BEAMS];

#define MAX_VISEDICTS 256
    extern int32_t      cl_numvisedicts;
    extern r_Entity_p   cl_visedicts[MAX_VISEDICTS];

    // cl_input
    typedef struct {
        uint8_t down[2];    // key nums holding it down
        uint8_t state;      // low bit is down state
    } kbutton_t;
    typedef kbutton_t* kbutton_p;

    extern  kbutton_t   in_mlook, in_klook;
    extern  kbutton_t   in_strafe;
    extern  kbutton_t   in_speed;

extern kbutton_t in_forward, in_forward2, in_back;

//=============================================================================
#ifdef __cplusplus
extern "C" {
#endif
    //
    // cl_main
    //
    dLight_p CL_AllocDlight(int32_t key);
    void CL_DecayLights();

    void CL_Init();

    void CL_EstablishConnection(cString host);
    void CL_Signon1();
    void CL_Signon2();
    void CL_Signon3();
    void CL_Signon4();

    void CL_Disconnect();
    void CL_Disconnect_f();
    void CL_NextDemo();


    void CL_InitInput();
    void CL_SendCmd();
    void CL_SendMove(UserCmd_p cmd);

    void CL_ParseTEnt();
    void CL_UpdateTEnts();

    void CL_ClearState();


    void CL_ReadFromServer();
    void CL_WriteToServer(UserCmd_p cmd);
    void CL_BaseMove(UserCmd_p cmd);


    float CL_KeyState(kbutton_p key);
    cString Key_KeynumToString(keycode_t keynum);

    // cl_demo.c
    void CL_StopPlayback();
    int CL_GetMessage();

    void CL_Stop_f();
    void CL_Record_f();
    void CL_PlayDemo_f();
    void CL_TimeDemo_f();

    // cl_parse.c
    void CL_ParseServerMessage();
    void CL_NewTranslation(int32_t slot);

    // view
    void V_StartPitchDrift();
    void V_StopPitchDrift();

    void V_RenderView();
    void V_UpdatePalette();
    void V_Register();
    void V_ParseDamage();
    void V_SetContentsColor(contents_t contents);


    // cl_tent
    void CL_InitTEnts();
    void CL_SignonReply();

#ifdef __cplusplus
}
#endif