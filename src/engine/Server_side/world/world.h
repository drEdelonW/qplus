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
// world.h
#include "vector.h"
#include "types.h"
#include "Edict.h"
#include "bspfile.h"
#include "Model_st.h"
#ifdef GLQUAKE
#   include "gl_model.h"
#   include "glquake.h"
#else
// #   include "model/model.h"
#endif

typedef struct {
    vec3_t  normal;
    float   dist;
} Plane_t;

typedef struct {
    bool    allsolid;   // if true, plane is not valid
    bool    startsolid; // if true, the initial point was in a solid area
    bool    inopen, inwater;
    float   fraction;   // time completed, 1.0 = didn't hit anything
    vec3_t  endpos;     // final position
    Plane_t plane;      // surface normal at impact
    edict_p ent;        // entity the surface is on
} trace_t;
typedef trace_t* trace_p;

typedef enum {
    MOVE_NORMAL     = 0u, // normal movement, collide with everything
    MOVE_NOMONSTERS = 1u, // ignore monsters
    MOVE_MISSILE    = 2u  // special missile movement rules
} phymovetype_t;

bool SV_RecursiveHullCheck(Hull_p hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_p trace);

void SV_ClearWorld();
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEdict(edict_p ent);
// call before removing an entity, and before trying to move one, so it doesn't clip against itself
// flags ent->v.modified

void SV_LinkEdict(edict_p ent, bool touch_triggers);
// Needs to be called any time an entity changes origin, mins, maxs, or solid flags ent->v.modified
// sets ent->v.absmin and ent->v.absmax
// if touchtriggers, calls prog functions for the intersected triggers

contents_t SV_PointContents(vec3_t p);
contents_t SV_TruePointContents(vec3_t p);
// returns the CONTENTS_* value from the world at the given point.
// does not check any entities at all
// the non-true version remaps the water current contents to content_water

edict_p SV_TestEntityPosition(edict_p ent);

trace_t SV_Move(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, phymovetype_t type, edict_p passedict);
// mins and maxs are reletive

// if the entire move stays in a solid volume, trace.allsolid will be set

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// nomonsters is used for line of sight or edge testing, where mosnters
// shouldn't be considered solid objects

// passedict is explicitly excluded from clipping checks (normally NULL)


int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, mPlane_p plane);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)      \
    (((p)->type < 3)? (                         \
        ((p)->dist <= (emins)[(p)->type])?      \
            1 : (                               \
            ((p)->dist >= (emaxs)[(p)->type])?  \
                2 : 3                           \
        )                                       \
    ) : BoxOnPlaneSide( (emins), (emaxs), (p)))
