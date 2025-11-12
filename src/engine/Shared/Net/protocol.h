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
	FAST_MASK = 0x7Fu,
	FAST_FLAG = 0x80u
};

// if the high bit of the servercmd is set, the low bits are fast update flags:

typedef enum {
    U_MOREBITS    = (1u << 0),
    U_ORIGIN1     = (1u << 1),
    U_ORIGIN2     = (1u << 2),
    U_ORIGIN3     = (1u << 3),
    U_ANGLE2      = (1u << 4),
    U_NOLERP      = (1u << 5),  // don't interpolate movement
    U_FRAME       = (1u << 6),
    U_SIGNAL      = (1u << 7),  // differentiates from other updates

    // svc_update can pass all of the fast update bits, plus more:
    U_ANGLE1      = (1u << 8),
    U_ANGLE3      = (1u << 9),
    U_MODEL       = (1u << 10),
    U_COLORMAP    = (1u << 11),
    U_SKIN        = (1u << 12),
    U_EFFECTS     = (1u << 13),
    U_LONGENTITY  = (1u << 14)
} update_bits_t;


typedef enum {
    SU_VIEWHEIGHT   = (1u << 0),
    SU_IDEALPITCH   = (1u << 1),
    SU_PUNCH1       = (1u << 2),
    SU_PUNCH2       = (1u << 3),
    SU_PUNCH3       = (1u << 4),
    SU_VELOCITY1    = (1u << 5),
    SU_VELOCITY2    = (1u << 6),
    SU_VELOCITY3    = (1u << 7),
    // SU_AIMENT    = (1u << 8),  // available bit
    SU_ITEMS        = (1u << 9),
    SU_ONGROUND     = (1u << 10), // no data follows
    SU_INWATER      = (1u << 11), // no data follows
    SU_WEAPONFRAME  = (1u << 12),
    SU_ARMOR        = (1u << 13),
    SU_WEAPON       = (1u << 14)
} server_update_bits_t;

// a sound with no channel is a local only sound

typedef enum {
    SND_VOLUME      = (1u << 0), // a byte
    SND_ATTENUATION = (1u << 1), // a byte
    SND_LOOPING     = (1u << 2)  // a long
} sound_bits_t;

// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	22


// game types sent by serverinfo
// these determine which intermission screen plays
typedef enum {
    GAME_COOP       = 0u, // cooperative mode
    GAME_DEATHMATCH = 1u  // deathmatch mode
} game_type_t;
//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//

typedef enum {
    svc_bad              = 0u,
    svc_nop              = 1u,
    svc_disconnect       = 2u,
    svc_updatestat       = 3u,   // [byte] [long]
    svc_version          = 4u,   // [long] server version
    svc_setview          = 5u,   // [short] entity number
    svc_sound            = 6u,   // <see code>
    svc_time             = 7u,   // [float] server time
    svc_print            = 8u,   // [string] null terminated string
    svc_stufftext        = 9u,   // [string] stuffed into client's console buffer
                                // the string should be \n terminated
    svc_setangle         = 10u,  // [angle3] set the view angle to this absolute value

    svc_serverinfo       = 11u,  // [long] version
                                // [string] signon string
                                // [string]..[0]model cache
                                // [string]...[0]sounds cache
    svc_lightstyle       = 12u,  // [byte] [string]
    svc_updatename       = 13u,  // [byte] [string]
    svc_updatefrags      = 14u,  // [byte] [short]
    svc_clientdata       = 15u,  // <shortbits + data>
    svc_stopsound        = 16u,  // <see code>
    svc_updatecolors     = 17u,  // [byte] [byte]
    svc_particle         = 18u,  // [vec3] <variable>
    svc_damage           = 19u,

    svc_spawnstatic      = 20u,
    // svc_spawnbinary      = 21u,  // unused
    svc_spawnbaseline    = 22u,

    svc_temp_entity      = 23u,
    svc_setpause         = 24u, // [byte] on/off
    svc_signonnum        = 25u, // [byte] used for signon sequence
    svc_centerprint      = 26u, // [string] center of the screen

    svc_killedmonster    = 27u,
    svc_foundsecret      = 28u,
    svc_spawnstaticsound = 29u, // [coord3] [byte] samp [byte] vol [byte] aten

    svc_intermission     = 30u, // [string] music
    svc_finale           = 31u, // [string] music [string] text

    svc_cdtrack          = 32u, // [byte] track [byte] looptrack
    svc_sellscreen       = 33u,
    svc_cutscene         = 34u

} svc_t;

//
// client to server
//

typedef enum {
    clc_bad         = 0u,
    clc_nop         = 1u,
    clc_disconnect  = 2u,
    clc_move        = 3u, // [UserCmd_t]
    clc_stringcmd   = 4u  // [string] message
} clc_t;



//
// stats are integers communicated to the client by the server
//
typedef enum {
    STAT_HEALTH         = 0u,
    STAT_FRAGS          = 1u,
    STAT_WEAPON         = 2u,
    STAT_AMMO           = 3u,
    STAT_ARMOR          = 4u,
    STAT_WEAPONFRAME    = 5u,
    STAT_SHELLS         = 6u,
    STAT_NAILS          = 7u,
    STAT_ROCKETS        = 8u,
    STAT_CELLS          = 9u,
    STAT_ACTIVEWEAPON   = 10u,
    STAT_TOTALSECRETS   = 11u,
    STAT_TOTALMONSTERS  = 12u,
    STAT_SECRETS        = 13u, // bumped client-side by svc_foundsecret
    STAT_MONSTERS       = 14u,  // bumped by svc_killedmonster

    MAX_CL_STATS        = 32u
} stat_t;
