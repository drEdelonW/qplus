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
#include "types.h"
#include "vector.h"

// VM global offsets; vectors occupy 3 float slots
typedef enum {
    OFS_NULL      = 0u,
    OFS_RETURN    = 1u,     // +3
    OFS_PARM0     = 4u,     // parm0..parm7: +3 per parm
    OFS_PARM1     = 7u,
    OFS_PARM2     = 10u,
    OFS_PARM3     = 13u,
    OFS_PARM4     = 16u,
    OFS_PARM5     = 19u,
    OFS_PARM6     = 22u,
    OFS_PARM7     = 25u,
    RESERVED_OFS  = 28u
} PrOfs_e;

// edict->movetype values
typedef enum {
    MOVETYPE_NONE           = 0u,  // never moves
    MOVETYPE_ANGLENOCLIP    = 1u,
    MOVETYPE_ANGLECLIP      = 2u,
    MOVETYPE_WALK           = 3u,  // gravity
    MOVETYPE_STEP           = 4u,  // gravity, special edge handling
    MOVETYPE_FLY            = 5u,
    MOVETYPE_TOSS           = 6u,  // gravity
    MOVETYPE_PUSH           = 7u,  // no clip to world, push and crush
    MOVETYPE_NOCLIP         = 8u,
    MOVETYPE_FLYMISSILE     = 9u,  // extra size to monsters
    MOVETYPE_BOUNCE         = 10u,
#ifdef QUAKE2
    MOVETYPE_BOUNCEMISSILE  = 11u, // bounce w/o gravity
    MOVETYPE_FOLLOW         = 12u, // track movement of aiment
#endif
} movetype_t;


// edict->solid values
typedef enum {
    SOLID_NOT       = 0u, // no interaction with other objects
    SOLID_TRIGGER   = 1u, // touch on edge, but not blocking
    SOLID_BBOX      = 2u, // touch on edge, block
    SOLID_SLIDEBOX  = 3u, // touch on edge, but not an onground
    SOLID_BSP       = 4u  // BSP clip, touch on edge, block
} solid_t;

// edict->deadflag values
typedef enum {
    DEAD_NO     = 0u, // alive
    DEAD_DYING  = 1u, // in the process of dying
    DEAD_DEAD   = 2u  // fully dead
} deadflag_t;

typedef enum {
    DAMAGE_NO   = 0u, // does not take damage
    DAMAGE_YES  = 1u, // always takes damage
    DAMAGE_AIM  = 2u  // takes damage only with aim
} damage_t;


// edict->flags
typedef enum {
    FL_FLY              = 1u << 0,   // 0000...0001
    FL_SWIM             = 1u << 1,   // 0000...0010
    // FL_GLIMPSE        = 1u << 1,
    FL_CONVEYOR         = 1u << 2,   // 0000...0100
    FL_CLIENT           = 1u << 3,   // 0000...1000
    FL_INWATER          = 1u << 4,   // 0001...0000
    FL_MONSTER          = 1u << 5,   // 0010...0000
    FL_GODMODE          = 1u << 6,
    FL_NOTARGET         = 1u << 7,
    FL_ITEM             = 1u << 8,
    FL_ONGROUND         = 1u << 9,
    FL_PARTIALGROUND    = 1u << 10,  // not all corners are valid
    FL_WATERJUMP        = 1u << 11,  // player jumping out of water
    FL_JUMPRELEASED     = 1u << 12,  // for jump debouncing
#ifdef QUAKE2
    FL_FLASHLIGHT       = 1u << 13,
    FL_ARCHIVE_OVERRIDE = 1u << 20
#endif
} EntityFlags_t;


typedef enum {
    SPAWNFLAG_NOT_EASY          = 1u << 8,  // 0x0100
    SPAWNFLAG_NOT_MEDIUM        = 1u << 9,  // 0x0200
    SPAWNFLAG_NOT_HARD          = 1u << 10, // 0x0400
    SPAWNFLAG_NOT_DEATHMATCH    = 1u << 11  // 0x0800
} SpawnFlags_t;

#ifdef QUAKE2
// server flags
typedef enum {
    SFL_EPISODE_1       = 1u << 0,   // 0x0001
    SFL_EPISODE_2       = 1u << 1,   // 0x0002
    SFL_EPISODE_3       = 1u << 2,   // 0x0004
    SFL_EPISODE_4       = 1u << 3,   // 0x0008
    SFL_NEW_UNIT        = 1u << 4,   // 0x0010
    SFL_NEW_EPISODE     = 1u << 5,   // 0x0020
    SFL_CROSS_TRIGGERS  = 0xFF00u    // 65280, covers multiple bits
} spawnlevel_flags_t;
#endif
typedef int32_t string_t;
typedef int32_t func_t;

#ifndef QUAKE2
#   include "progdefs.q1"
#else
#   include "progdefs.q2"
#endif

typedef globalvars_t* globalvars_p;
extern globalvars_p pr_global_struct;   // global variable of game settings
extern float_p      pr_globals;         // same as pr_global_struct

#define RETURN_EDICT(edict) (((int *)pr_globals)[OFS_RETURN] = ED_GetEDictOffs(edict))
#define G_FLOAT(o)          (pr_globals[(o)])
#define G_INT(o)            (*(int32_p)&pr_globals[(o)])
#define G_VECTOR(o)         (&pr_globals[(o)])
#define G_STRING(o)         PR_GetQString(*(qVmString_t*)&pr_globals[(o)])

typedef entvars_t* entvars_p;