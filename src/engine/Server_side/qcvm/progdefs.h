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
#include "vector.h"


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
    MOVETYPE_BOUNCEMISSILE  = 11, // bounce w/o gravity
    MOVETYPE_FOLLOW         = 12, // track movement of aiment
#endif
} movetype_t;


// edict->solid values
typedef enum {
    SOLID_NOT       = 0, // no interaction with other objects
    SOLID_TRIGGER   = 1, // touch on edge, but not blocking
    SOLID_BBOX      = 2, // touch on edge, block
    SOLID_SLIDEBOX  = 3, // touch on edge, but not an onground
    SOLID_BSP       = 4  // BSP clip, touch on edge, block
} solid_t;

// edict->deadflag values
typedef enum {
    DEAD_NO     = 0, // alive
    DEAD_DYING  = 1, // in the process of dying
    DEAD_DEAD   = 2  // fully dead
} deadflag_t;

typedef enum {
    DAMAGE_NO   = 0, // does not take damage
    DAMAGE_YES  = 1, // always takes damage
    DAMAGE_AIM  = 2  // takes damage only with aim
} damage_t;


// edict->flags
typedef enum {
    FL_FLY =            1 << 0,   // 0000...0001
    FL_SWIM             = 1 << 1,   // 0000...0010
    // FL_GLIMPSE        = 1 << 1,
    FL_CONVEYOR         = 1 << 2,   // 0000...0100
    FL_CLIENT           = 1 << 3,   // 0000...1000
    FL_INWATER          = 1 << 4,   // 0001...0000
    FL_MONSTER          = 1 << 5,   // 0010...0000
    FL_GODMODE          = 1 << 6,
    FL_NOTARGET         = 1 << 7,
    FL_ITEM             = 1 << 8,
    FL_ONGROUND         = 1 << 9,
    FL_PARTIALGROUND    = 1 << 10,  // not all corners are valid
    FL_WATERJUMP        = 1 << 11,  // player jumping out of water
    FL_JUMPRELEASED     = 1 << 12,  // for jump debouncing
#ifdef QUAKE2
    FL_FLASHLIGHT       = 1 << 13,
    FL_ARCHIVE_OVERRIDE = 1 << 20
#endif
} EntityFlags_t;


typedef enum {
    SPAWNFLAG_NOT_EASY          = 1 << 8,  // 0x0100
    SPAWNFLAG_NOT_MEDIUM        = 1 << 9,  // 0x0200
    SPAWNFLAG_NOT_HARD          = 1 << 10, // 0x0400
    SPAWNFLAG_NOT_DEATHMATCH    = 1 << 11  // 0x0800
} SpawnFlags_t;

#ifdef QUAKE2
// server flags
typedef enum {
    SFL_EPISODE_1       = 1 << 0,   // 0x0001
    SFL_EPISODE_2       = 1 << 1,   // 0x0002
    SFL_EPISODE_3       = 1 << 2,   // 0x0004
    SFL_EPISODE_4       = 1 << 3,   // 0x0008
    SFL_NEW_UNIT        = 1 << 4,   // 0x0010
    SFL_NEW_EPISODE     = 1 << 5,   // 0x0020
    SFL_CROSS_TRIGGERS  = 0xFF00    // 65280, covers multiple bits
} spawnlevel_flags_t;
#endif

#ifndef QUAKE2
#   include "progdefs.q1"
#else
#   include "progdefs.q2"
#endif

typedef globalvars_t* globalvars_p;
typedef entvars_t* entvars_p;