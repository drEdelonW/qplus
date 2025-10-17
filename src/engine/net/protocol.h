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
#if 0
#define	U_MOREBITS	(1<<0)
#define	U_ORIGIN1	(1<<1)
#define	U_ORIGIN2	(1<<2)
#define	U_ORIGIN3	(1<<3)
#define	U_ANGLE2	(1<<4)
#define	U_NOLERP	(1<<5)		// don't interpolate movement
#define	U_FRAME		(1<<6)
#define U_SIGNAL	(1<<7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define	U_ANGLE1	(1<<8)
#define	U_ANGLE3	(1<<9)
#define	U_MODEL		(1<<10)
#define	U_COLORMAP	(1<<11)
#define	U_SKIN		(1<<12)
#define	U_EFFECTS	(1<<13)
#define	U_LONGENTITY	(1<<14)
#else
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
#endif

#if 0
#define	SU_VIEWHEIGHT	(1<<0)
#define	SU_IDEALPITCH	(1<<1)
#define	SU_PUNCH1		(1<<2)
#define	SU_PUNCH2		(1<<3)
#define	SU_PUNCH3		(1<<4)
#define	SU_VELOCITY1	(1<<5)
#define	SU_VELOCITY2	(1<<6)
#define	SU_VELOCITY3	(1<<7)
//define	SU_AIMENT		(1<<8)  AVAILABLE BIT
#define	SU_ITEMS		(1<<9)
#define	SU_ONGROUND		(1<<10)		// no data follows, the bit is it
#define	SU_INWATER		(1<<11)		// no data follows, the bit is it
#define	SU_WEAPONFRAME	(1<<12)
#define	SU_ARMOR		(1<<13)
#define	SU_WEAPON		(1<<14)
#else
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
#endif

// a sound with no channel is a local only sound
#if 0
#define	SND_VOLUME		(1<<0)		// a byte
#define	SND_ATTENUATION	(1<<1)		// a byte
#define	SND_LOOPING		(1<<2)		// a long
#else
typedef enum {
    SND_VOLUME      = (1 << 0), // a byte
    SND_ATTENUATION = (1 << 1), // a byte
    SND_LOOPING     = (1 << 2)  // a long
} sound_bits_t;
#endif

// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	22


// game types sent by serverinfo
// these determine which intermission screen plays
#if 0
#define	GAME_COOP			0
#define	GAME_DEATHMATCH		1
#else
typedef enum {
    GAME_COOP       = 0, // cooperative mode
    GAME_DEATHMATCH = 1  // deathmatch mode
} game_type_t;
#endif
//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
#if 0
#define	svc_bad				0
#define	svc_nop				1
#define	svc_disconnect		2
#define	svc_updatestat		3	// [byte] [long]
#define	svc_version			4	// [long] server version
#define	svc_setview			5	// [short] entity number
#define	svc_sound			6	// <see code>
#define	svc_time			7	// [float] server time
#define	svc_print			8	// [string] null terminated string
#define	svc_stufftext		9	// [string] stuffed into client's console buffer
								// the string should be \n terminated
#define	svc_setangle		10	// [angle3] set the view angle to this absolute value

#define	svc_serverinfo		11	// [long] version
						// [string] signon string
						// [string]..[0]model cache
						// [string]...[0]sounds cache
#define	svc_lightstyle		12	// [byte] [string]
#define	svc_updatename		13	// [byte] [string]
#define	svc_updatefrags		14	// [byte] [short]
#define	svc_clientdata		15	// <shortbits + data>
#define	svc_stopsound		16	// <see code>
#define	svc_updatecolors	17	// [byte] [byte]
#define	svc_particle		18	// [vec3] <variable>
#define	svc_damage			19

#define	svc_spawnstatic		20
//	svc_spawnbinary		21
#define	svc_spawnbaseline	22

#define	svc_temp_entity		23

#define	svc_setpause		24	// [byte] on / off
#define	svc_signonnum		25	// [byte]  used for the signon sequence

#define	svc_centerprint		26	// [string] to put in center of the screen

#define	svc_killedmonster	27
#define	svc_foundsecret		28

#define	svc_spawnstaticsound	29	// [coord3] [byte] samp [byte] vol [byte] aten

#define	svc_intermission	30		// [string] music
#define	svc_finale			31		// [string] music [string] text

#define	svc_cdtrack			32		// [byte] track [byte] looptrack
#define svc_sellscreen		33

#define svc_cutscene		34
#else
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
#endif

//
// client to server
//
#if 0
#define	clc_bad			0
#define	clc_nop 		1
#define	clc_disconnect	2
#define	clc_move		3			// [UserCmd_t]
#define	clc_stringcmd	4		// [string] message
#else
typedef enum {
    clc_bad         = 0,
    clc_nop         = 1,
    clc_disconnect  = 2,
    clc_move        = 3, // [UserCmd_t]
    clc_stringcmd   = 4  // [string] message
} clc_t;
#endif

//
// temp entity events
//
#if 0
#define	TE_SPIKE			0
#define	TE_SUPERSPIKE		1
#define	TE_GUNSHOT			2
#define	TE_EXPLOSION		3
#define	TE_TAREXPLOSION		4
#define	TE_LIGHTNING1		5
#define	TE_LIGHTNING2		6
#define	TE_WIZSPIKE			7
#define	TE_KNIGHTSPIKE		8
#define	TE_LIGHTNING3		9
#define	TE_LAVASPLASH		10
#define	TE_TELEPORT			11
#define TE_EXPLOSION2		12

// PGM 01/21/97
#define TE_BEAM				13
// PGM 01/21/97

#ifdef QUAKE2
#define TE_IMPLOSION		14
#define TE_RAILTRAIL		15
#endif
#else
typedef enum {
    TE_SPIKE        = 0,
    TE_SUPERSPIKE   = 1,
    TE_GUNSHOT      = 2,
    TE_EXPLOSION    = 3,
    TE_TAREXPLOSION = 4,
    TE_LIGHTNING1   = 5,
    TE_LIGHTNING2   = 6,
    TE_WIZSPIKE     = 7,
    TE_KNIGHTSPIKE  = 8,
    TE_LIGHTNING3   = 9,
    TE_LAVASPLASH   = 10,
    TE_TELEPORT     = 11,
    TE_EXPLOSION2   = 12,

    // PGM 01/21/97
    TE_BEAM         = 13,
    // PGM 01/21/97

#ifdef QUAKE2
    TE_IMPLOSION    = 14,
    TE_RAILTRAIL    = 15,
#endif
} te_t;
#endif
