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

#include "Model_st.h"
#ifdef GLQUAKE
#   include "qOpenGL.h"
#endif

#define DIST_EPSILON (0.03125f) /* 1/32 epsilon to keep floating point happy */

void SV_ClearWorld();

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

