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

#include "world_priv.h"


/*
============
SV_TestEntityPosition

This could be a lot more efficient...
============
*/
#include "BBox_tools.h"
edict_p SV_TestEntityPosition(edict_p ent) {
    trace_t trace = SV_Move(
        ent->v.origin, EvBBox(&ent->v), ent->v.origin,
        MOVE_NORMAL, ent
    );

    return (trace.startsolid) ? GetEdictsPtr() : NULL; // world map or none
}




/*
==================
SV_ClipMoveToEntity

Handles selection or creation of a clipping hull, and offseting (and
eventually rotation) of the end points
==================
*/
trace_t SV_ClipMoveToEntity(edict_p ent, vec3_t start, BBox_t bb, vec3_t end) {
    // fill in a default trace
    trace_t trace = {
        .fraction = 1.f,
        .allsolid = true,
        .endpos = end
    };

    // get the clipping hull
    vec3_t offset;
    Hull_p hull = SV_HullForEntity(ent, bb, &offset);
    vec3_t start_l = VectorSubtract(start, offset);
    vec3_t end_l = VectorSubtract(end, offset);

#ifdef QUAKE2
    // rotate start and end into the models frame of reference
    if (((solid_t)ent->v.solid == SOLID_BSP) &&
        (
            ent->v.angles.pitch ||
            ent->v.angles.yaw ||
            ent->v.angles.roll)
        ) {
        // vec3_t a;
        vec3_t forward, right, up; AngleVectors(ent->v.angles, &forward, &right, &up);

        start_l = (vec3_t){
            .x = DotProduct(start_l, forward);
            .y = -DotProduct(start_l, right);
            .z = DotProduct(start_l, up);
        };

        end_l = (vec3_t){
            .x = DotProduct(end_l, forward);
            .y = -DotProduct(end_l, right);
            .z = DotProduct(end_l, up);
        };
    }
#endif

    // trace a line through the apropriate clipping hull
    SV_RecursiveHullCheck(hull, hull->firstclipnode, 0.f, 1.f, start_l, end_l, &trace);

#ifdef QUAKE2
    // rotate endpos back to world frame of reference
    if (((solid_t)ent->v.solid == SOLID_BSP) &&
        (
            ent->v.angles.pitch ||
            ent->v.angles.yaw ||
            ent->v.angles.roll) &&
        (trace.fraction != 1)) {
        vec3_t forward, right, up;  AngleVectors(
            VectorSubtract(v3Zero, ent->v.angles),
            &forward, &right, &up
        );

        trace.endpos = (vec3_t){
            .x = DotProduct(trace.endpos, forward),
            .y = -DotProduct(trace.endpos, right),
            .z = DotProduct(trace.endpos, up),
        };

        trace.plane.normal = (vec3_t){
            .x = DotProduct(trace.plane.normal, forward),
            .y = -DotProduct(trace.plane.normal, right),
            .z = DotProduct(trace.plane.normal, up),
        };

    }
#endif

    // fix trace up by the offset
    if (trace.fraction != 1.f)
        trace.endpos = VectorAdd(trace.endpos, offset);

    // did we clip the move?
    if ((trace.fraction < 1.f) ||
        (trace.startsolid)
        )   trace.pEnt = ent;

    return trace;
}

//===========================================================================


/*
====================
SV_ClipToLinks

Mins and maxs enclose the entire area swept by the move
====================
*/
void SV_ClipToLinks(areaNode_p node, moveClip_p clip) {
    // touch linked edicts
    link_p next;
    for (link_p l = node->solid_edicts.next; l != &node->solid_edicts; l = next) {
        next = l->next;
        edict_p touch = EDICT_FROM_AREA(l);
        if ((touch->v.solid == SOLID_NOT) ||
            (touch == clip->passedict)
            )   continue;

        if (touch->v.solid == SOLID_TRIGGER)    Host_SysError("Trigger in clipping list");

        if ((clip->moveType == MOVE_NOMONSTERS) &&
            (touch->v.solid != SOLID_BSP)
            )   continue;

        if (
            (
                clip->box.mins.x > touch->v.absmax.x ||
                clip->box.mins.y > touch->v.absmax.y ||
                clip->box.mins.z > touch->v.absmax.z) ||
            (
                clip->box.maxs.x < touch->v.absmin.x ||
                clip->box.maxs.y < touch->v.absmin.y ||
                clip->box.maxs.z < touch->v.absmin.z) ||
            (
                clip->passedict &&
                clip->passedict->v.size.x &&
                !touch->v.size.x
                )
            )   continue; // points never interact

        // might intersect, so do an exact clip
        if (clip->trace.allsolid)   return;

        if (clip->passedict) {
            if ((ED_GetEDictByOffs(touch->v.owner) == clip->passedict) || // don't clip against own missiles
                (ED_GetEDictByOffs(clip->passedict->v.owner) == touch)) // don't clip against owner
                continue;
        }

        trace_t trace;
        if ((int)touch->v.flags & FL_MONSTER)       trace = SV_ClipMoveToEntity(touch, clip->start, clip->m2, clip->end);
        else                                        trace = SV_ClipMoveToEntity(touch, clip->start, clip->mv, clip->end);

        if (trace.allsolid ||
            trace.startsolid ||
            (trace.fraction < clip->trace.fraction)
            ) {
            trace.pEnt = touch;
            if (clip->trace.startsolid) {
                clip->trace = trace;
                clip->trace.startsolid = true;
            }
            else clip->trace = trace;
        }
        else if (trace.startsolid)
            clip->trace.startsolid = true;
    }

    // recurse down both sides
    if (node->axis == AXIS_LEAF)
        return;

    if (clip->box.maxs.v[node->axis] > node->dist)     SV_ClipToLinks(node->children[0], clip);
    if (clip->box.mins.v[node->axis] < node->dist)     SV_ClipToLinks(node->children[1], clip);
}



void SV_MoveBounds(vec3_t start, BBox_t bb, vec3_t end, BBox_p box) {
#if 0
    // debug to test against everything
    box = bbNull;
#else
    for (int i = 0; i < VECT_DIM; i++) {
        if (end.v[i] > start.v[i]) {
            box->mins.v[i] = start.v[i] + bb.mins.v[i] - 1;
            box->maxs.v[i] = end.v[i] + bb.maxs.v[i] + 1;
        }
        else {
            box->mins.v[i] = end.v[i] + bb.mins.v[i] - 1;
            box->maxs.v[i] = start.v[i] + bb.maxs.v[i] + 1;
        }
    }
#endif
}


trace_t SV_Move(vec3_t start, BBox_t bb, vec3_t end, phymovetype_t type, edict_p passedict) {
    moveClip_t clip = {
        // .box = ,
        .mv = bb,
        .m2 = (type == MOVE_MISSILE) ?
            BBoxSymmetric(15.f) : bb,
        .start = start,
        .end = end,

        .trace = SV_ClipMoveToEntity(GetEdictsPtr(), start, bb, end),
        .moveType = type,
        .passedict = passedict
    };

    SV_MoveBounds(start, clip.m2, end, &clip.box);  // create the bounding box of the entire move
    SV_ClipToLinks(_sv_AreaNodes, &clip); // clip to entities

    return clip.trace;
}
