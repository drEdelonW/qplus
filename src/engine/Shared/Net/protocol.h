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
// protocol.h -- communications protocols

#define	PROTOCOL_VERSION	15
enum {
	FAST_MASK = 0x7F,
	FAST_FLAG = 0x80
};

// if the high bit of the servercmd is set, the low bits are fast update flags:

typedef enum {
    U_MOREBITS    = (1 << 0),
    U_ORIGIN1     = (1 << 1),
    U_ORIGIN2     = (1 << 2),
    U_ORIGIN3     = (1 << 3),
    U_ANGLE2      = (1 << 4),
    U_NOLERP      = (1 << 5),  // don't interpolate movement
    U_FRAME       = (1 << 6),
    U_SIGNAL      = (1 << 7),  // differentiates from other updates

    // svc_update can pass all of the fast update bits, plus more:
    U_ANGLE1      = (1 << 8),
    U_ANGLE3      = (1 << 9),
    U_MODEL       = (1 << 10),
    U_COLORMAP    = (1 << 11),
    U_SKIN        = (1 << 12),
    U_EFFECTS     = (1 << 13),
    U_LONGENTITY  = (1 << 14)
} update_bits_t;


typedef enum {
    SU_VIEWHEIGHT   = (1 << 0),
    SU_IDEALPITCH   = (1 << 1),
    SU_PUNCH1       = (1 << 2),
    SU_PUNCH2       = (1 << 3),
    SU_PUNCH3       = (1 << 4),
    SU_VELOCITY1    = (1 << 5),
    SU_VELOCITY2    = (1 << 6),
    SU_VELOCITY3    = (1 << 7),
    // SU_AIMENT    = (1 << 8),  // available bit
    SU_ITEMS        = (1 << 9),
    SU_ONGROUND     = (1 << 10), // no data follows
    SU_INWATER      = (1 << 11), // no data follows
    SU_WEAPONFRAME  = (1 << 12),
    SU_ARMOR        = (1 << 13),
    SU_WEAPON       = (1 << 14)
} server_update_bits_t;

// a sound with no channel is a local only sound

typedef enum {
    SND_VOLUME      = (1 << 0), // a byte
    SND_ATTENUATION = (1 << 1), // a byte
    SND_LOOPING     = (1 << 2)  // a long
} sound_bits_t;

// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	22


// game types sent by serverinfo
// these determine which intermission screen plays
typedef enum {
    GAME_COOP       = 0, // cooperative mode
    GAME_DEATHMATCH = 1  // deathmatch mode
} game_type_t;
//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//

typedef enum {
    svc_bad              = 0,
    svc_nop              = 1,
    svc_disconnect       = 2,
    svc_updatestat       = 3,   // [byte] [long]
    svc_version          = 4,   // [long] server version
    svc_setview          = 5,   // [short] entity number
    svc_sound            = 6,   // <see code>
    svc_time             = 7,   // [float] server time
    svc_print            = 8,   // [string] null terminated string
    svc_stufftext        = 9,   // [string] stuffed into client's console buffer
                                // the string should be \n terminated
    svc_setangle         = 10,  // [angle3] set the view angle to this absolute value

    svc_serverinfo       = 11,  // [long] version
                                // [string] signon string
                                // [string]..[0]model cache
                                // [string]...[0]sounds cache
    svc_lightstyle       = 12,  // [byte] [string]
    svc_updatename       = 13,  // [byte] [string]
    svc_updatefrags      = 14,  // [byte] [short]
    svc_clientdata       = 15,  // <shortbits + data>
    svc_stopsound        = 16,  // <see code>
    svc_updatecolors     = 17,  // [byte] [byte]
    svc_particle         = 18,  // [vec3] <variable>
    svc_damage           = 19,

    svc_spawnstatic      = 20,
    // svc_spawnbinary      = 21,  // unused
    svc_spawnbaseline    = 22,

    svc_temp_entity      = 23,
    svc_setpause         = 24, // [byte] on/off
    svc_signonnum        = 25, // [byte] used for signon sequence
    svc_centerprint      = 26, // [string] center of the screen

    svc_killedmonster    = 27,
    svc_foundsecret      = 28,
    svc_spawnstaticsound = 29, // [coord3] [byte] samp [byte] vol [byte] aten

    svc_intermission     = 30, // [string] music
    svc_finale           = 31, // [string] music [string] text

    svc_cdtrack          = 32, // [byte] track [byte] looptrack
    svc_sellscreen       = 33,
    svc_cutscene         = 34

} svc_t;

//
// client to server
//

typedef enum {
    clc_bad         = 0,
    clc_nop         = 1,
    clc_disconnect  = 2,
    clc_move        = 3, // [UserCmd_t]
    clc_stringcmd   = 4  // [string] message
} clc_t;



//
// stats are integers communicated to the client by the server
//
typedef enum {
    STAT_HEALTH         = 0,
    STAT_FRAGS          = 1,
    STAT_WEAPON         = 2,
    STAT_AMMO           = 3,
    STAT_ARMOR          = 4,
    STAT_WEAPONFRAME    = 5,
    STAT_SHELLS         = 6,
    STAT_NAILS          = 7,
    STAT_ROCKETS        = 8,
    STAT_CELLS          = 9,
    STAT_ACTIVEWEAPON   = 10,
    STAT_TOTALSECRETS   = 11,
    STAT_TOTALMONSTERS  = 12,
    STAT_SECRETS        = 13, // bumped client-side by svc_foundsecret
    STAT_MONSTERS       = 14,  // bumped by svc_killedmonster

    MAX_CL_STATS        = 32
} stat_t;
