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
// world.c -- world query functions

#include "world.h"
#include <string.h>
#include "server.h"
#include "sys.h"
#include "console.h"
#include "q_tools.h"
#include "mathlib.h"
#include "progs.h"


/*
    entities never clip against themselves, or their owner
    line of sight checks trace->crosscontent, but bullets don't
*/

typedef struct {
    vec3_t  boxmins, boxmaxs;// enclose the test object along entire move
    float_p mins;
    float_p maxs; // size of the moving object
    vec3_t  mins2, maxs2; // size when clipping against mosnters
    float_p start;
    float_p end;
    trace_t trace;
    phymovetype_t type;
    edict_p passedict;
} moveClip_t;
typedef moveClip_t* moveClip_p;


/*
===============================================================================

HULL BOXES

===============================================================================
*/

static Hull_t       _boxHull;
static dClipNode_t  _boxClipNodes[6];
static mPlane_t     _boxPlanes[6];

/*
===================
SV_InitBoxHull

Set up the planes and clipnodes so that the six floats of a bounding box
can just be stored out and get a proper Hull_t structure.
===================
*/
void SV_InitBoxHull() {
    _boxHull.clipnodes = _boxClipNodes;
    _boxHull.planes = _boxPlanes;
    _boxHull.firstclipnode = 0;
    _boxHull.lastclipnode = 5;

    for (uint8_t i = 0; i < 6; i++) {
        _boxClipNodes[i].planenum = i;

        int side = i & 1;

        _boxClipNodes[i].children[side] = CONTENTS_EMPTY;
        if (i != 5)     _boxClipNodes[i].children[side ^ 1] = i + 1;
        else            _boxClipNodes[i].children[side ^ 1] = CONTENTS_SOLID;

        _boxPlanes[i].type = i >> 1;
        _boxPlanes[i].normal[i >> 1] = 1;
    }

}


/*
===================
SV_HullForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
Hull_p SV_HullForBox(vec3_t mins, vec3_t maxs) {
    _boxPlanes[0].dist = maxs[0];
    _boxPlanes[1].dist = mins[0];
    _boxPlanes[2].dist = maxs[1];
    _boxPlanes[3].dist = mins[1];
    _boxPlanes[4].dist = maxs[2];
    _boxPlanes[5].dist = mins[2];
    return &_boxHull;
}



/*
================
SV_HullForEntity

Returns a hull that can be used for testing or clipping an object of mins/maxs
size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
================
*/
Hull_p SV_HullForEntity(edict_p ent, vec3_t mins, vec3_t maxs, vec3_t offset) {
    Hull_p  hull;
    // decide which clipping hull to use, based on the size
    if (ent->v.solid == SOLID_BSP) { // explicit hulls in the BSP model
        if (ent->v.movetype != MOVETYPE_PUSH)
            Sys_Error("SOLID_BSP without MOVETYPE_PUSH");

        Model_p model = sv.models[(int)ent->v.modelindex];

        if (!model ||
            (model->type != mod_brush)
            )
            Sys_Error("MOVETYPE_PUSH with a non bsp model");

        vec3_t size; VectorSubtract(maxs, mins, size);
        if (size[0] < 3)        hull = &model->hulls[0];
        else if (size[0] <= 32) hull = &model->hulls[1];
        else                    hull = &model->hulls[2];

        // calculate an offset value to center the origin
        VectorSubtract(hull->clip_mins, mins, offset);
        VectorAdd(offset, ent->v.origin, offset);
    }
    else { // create a temp hull from bounding box sizes

        vec3_t hullmins; VectorSubtract(ent->v.mins, maxs, hullmins);
        vec3_t hullmaxs; VectorSubtract(ent->v.maxs, mins, hullmaxs);
        hull = SV_HullForBox(hullmins, hullmaxs);

        VectorCopy(ent->v.origin, offset);
    }

    return hull;
}

/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/

typedef struct areaNode_s areaNode_t;
typedef areaNode_t* areaNode_p;
typedef struct areaNode_s {
    int         axis;  // -1 = leaf node
    float       dist;
    areaNode_p  children[2];
    link_t      trigger_edicts;
    link_t      solid_edicts;
} areaNode_t;

#define AREA_DEPTH 4
#define AREA_NODES 32

static areaNode_t   _sv_AreaNodes[AREA_NODES];
static int          _sv_NumAreaNodes;

/*
===============
SV_CreateAreaNode

===============
*/
areaNode_p SV_CreateAreaNode(int depth, vec3_t mins, vec3_t maxs) {
    areaNode_p anode = &_sv_AreaNodes[_sv_NumAreaNodes];
    _sv_NumAreaNodes++;

    ClearLink(&anode->trigger_edicts);
    ClearLink(&anode->solid_edicts);

    if (depth == AREA_DEPTH) {
        anode->axis = -1;
        anode->children[0] = anode->children[1] = NULL;
        return anode;
    }

    vec3_t size; VectorSubtract(maxs, mins, size);
    if (size[0] > size[1])  anode->axis = 0;
    else                    anode->axis = 1;

    anode->dist = 0.5f * (maxs[anode->axis] + mins[anode->axis]);
    vec3_t mins1; VectorCopy(mins, mins1);
    vec3_t mins2; VectorCopy(mins, mins2);
    vec3_t maxs1; VectorCopy(maxs, maxs1);
    vec3_t maxs2; VectorCopy(maxs, maxs2);

    maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

    anode->children[0] = SV_CreateAreaNode(depth + 1, mins2, maxs2);
    anode->children[1] = SV_CreateAreaNode(depth + 1, mins1, maxs1);

    return anode;
}

/*
===============
SV_ClearWorld

===============
*/
void SV_ClearWorld() {
    SV_InitBoxHull();

    memset(_sv_AreaNodes, 0, sizeof(_sv_AreaNodes));
    _sv_NumAreaNodes = 0;
    SV_CreateAreaNode(0, sv.worldmodel->mins, sv.worldmodel->maxs);
}


/*
===============
SV_UnlinkEdict

===============
*/
void SV_UnlinkEdict(edict_p ent) {
    if (!ent->area.prev)    return;  // not linked in anywhere

    RemoveLink(&ent->area);
    ent->area.prev = ent->area.next = NULL;
}


/*
====================
SV_TouchLinks
====================
*/
void SV_TouchLinks(edict_p ent, areaNode_p node) {
    // touch linked edicts
    link_p next;
    for (link_p l = node->trigger_edicts.next; l != &node->trigger_edicts; l = next) {
        next = l->next;
        edict_p touch = EDICT_FROM_AREA(l);
        if (touch == ent)   continue;

        if (!touch->v.touch ||
            (touch->v.solid != SOLID_TRIGGER))
            continue;

        if ((ent->v.absmin[0] > touch->v.absmax[0]) ||
            (ent->v.absmin[1] > touch->v.absmax[1]) ||
            (ent->v.absmin[2] > touch->v.absmax[2]) ||
            (ent->v.absmax[0] < touch->v.absmin[0]) ||
            (ent->v.absmax[1] < touch->v.absmin[1]) ||
            (ent->v.absmax[2] < touch->v.absmin[2]))
            continue;

        int old_self = pr_global_struct->self;
        int old_other = pr_global_struct->other;

        pr_global_struct->self = EDICT_TO_PROG(touch);
        pr_global_struct->other = EDICT_TO_PROG(ent);
        pr_global_struct->time = (float)sv.time;
        PR_ExecuteProgram(touch->v.touch);

        pr_global_struct->self = old_self;
        pr_global_struct->other = old_other;
    }

    // recurse down both sides
    if (node->axis == -1)       return;

    if (ent->v.absmax[node->axis] > node->dist)     SV_TouchLinks(ent, node->children[0]);
    if (ent->v.absmin[node->axis] < node->dist)     SV_TouchLinks(ent, node->children[1]);
}


/*
===============
SV_FindTouchedLeafs

===============
*/
void SV_FindTouchedLeafs(edict_p ent, mNode_p node) {
    if (node->contents == CONTENTS_SOLID)       return;

    // add an efrag if the node is a leaf
    if (node->contents < 0) {
        if (ent->num_leafs == MAX_ENT_LEAFS)    return;

        mLeaf_p leaf = (mLeaf_p)node;
        int32_t leafnum = leaf - sv.worldmodel->leafs - 1;

        ent->leafnums[ent->num_leafs] = (int16_t)leafnum;
        ent->num_leafs++;
        return;
    }

    // NODE_MIXED
    mPlane_p splitplane = node->plane;
    int sides = BOX_ON_PLANE_SIDE(
        ent->v.absmin,
        ent->v.absmax,
        splitplane
    );

    // recurse down the contacted sides
    if (sides & 1)      SV_FindTouchedLeafs(ent, node->children[0]);
    if (sides & 2)      SV_FindTouchedLeafs(ent, node->children[1]);
}

/*
===============
SV_LinkEdict

===============
*/
void SV_LinkEdict(edict_p ent, bool touch_triggers) {
    if (ent->area.prev)
        SV_UnlinkEdict(ent); // unlink from old position

    if ((ent == sv.edicts) ||  // don't add the world
        (ent->free))
        return;

    // set the abs box

#ifdef QUAKE2
    if (ent->v.solid == SOLID_BSP &&
        (
            ent->v.angles[0] ||
            ent->v.angles[1] ||
            ent->v.angles[2]
            )) { // expand for rotation
        float max = 0;
        for (int i = 0; i < VECT_DIM; i++) {
            float v = fabs(ent->v.mins[i]);
            if (v > max)
                max = v;
            v = fabs(ent->v.maxs[i]);
            if (v > max)
                max = v;
        }
        for (int i = 0; i < VECT_DIM; i++) {
            ent->v.absmin[i] = ent->v.origin[i] - max;
            ent->v.absmax[i] = ent->v.origin[i] + max;
        }
    }
    else
#endif
    {
        VectorAdd(ent->v.origin, ent->v.mins, ent->v.absmin);
        VectorAdd(ent->v.origin, ent->v.maxs, ent->v.absmax);
    }

    //
    // to make items easier to pick up and allow them to be grabbed off
    // of shelves, the abs sizes are expanded
    //
    if ((int)ent->v.flags & FL_ITEM) {
        ent->v.absmin[0] -= 15;
        ent->v.absmin[1] -= 15;

        ent->v.absmax[0] += 15;
        ent->v.absmax[1] += 15;
    }
    else { // because movement is clipped an epsilon away from an actual edge,
        // we must fully check even when bounding boxes don't quite touch
        ent->v.absmin[0] -= 1;
        ent->v.absmin[1] -= 1;
        ent->v.absmin[2] -= 1;

        ent->v.absmax[0] += 1;
        ent->v.absmax[1] += 1;
        ent->v.absmax[2] += 1;
    }

    // link to PVS leafs
    ent->num_leafs = 0;
    if (ent->v.modelindex)
        SV_FindTouchedLeafs(ent, sv.worldmodel->nodes);

    if (ent->v.solid == SOLID_NOT)      return;

    // find the first node that the ent's box crosses
    areaNode_p node = _sv_AreaNodes;
    while (1) {
        if (node->axis == -1)       break;

        if (ent->v.absmin[node->axis] > node->dist)         node = node->children[0];
        else if (ent->v.absmax[node->axis] < node->dist)    node = node->children[1];
        else    break;  // crosses the node
    }

    // link it in

    if (ent->v.solid == SOLID_TRIGGER)  InsertLinkBefore(&ent->area, &node->trigger_edicts);
    else                                InsertLinkBefore(&ent->area, &node->solid_edicts);

    // if touch_triggers, touch all entities at this node and decend for more
    if (touch_triggers)     SV_TouchLinks(ent, _sv_AreaNodes);
}



/*
===============================================================================

POINT TESTING IN HULLS

===============================================================================
*/

#if !id386

/*
==================
SV_HullPointContents

==================
*/
int SV_HullPointContents(Hull_p hull, int num, vec3_t p) {
    while (num >= 0) {
        if ((num < hull->firstclipnode) ||
            (num > hull->lastclipnode)
            )
            Sys_Error("SV_HullPointContents: bad node number");

        dClipNode_p node = hull->clipnodes + num;
        mPlane_p plane = hull->planes + node->planenum;

        float d =
            ((plane->type < 3) ?
                p[plane->type] : DotProduct(plane->normal, p)) -
            plane->dist;

        num = node->children[(d < 0) ? 1 : 0];
    }

    return num;
}

#endif // !id386


/*
==================
SV_PointContents

==================
*/
contents_t SV_PointContents(vec3_t p) {
    contents_t cont = SV_HullPointContents(&sv.worldmodel->hulls[0], 0, p);
    if ((cont <= CONTENTS_CURRENT_0) &&
        (cont >= CONTENTS_CURRENT_DOWN)
        )
        cont = CONTENTS_WATER;
    return cont;
}

contents_t SV_TruePointContents(vec3_t p) {
    return SV_HullPointContents(&sv.worldmodel->hulls[0], 0, p);
}

//===========================================================================

/*
============
SV_TestEntityPosition

This could be a lot more efficient...
============
*/
edict_p SV_TestEntityPosition(edict_p ent) {
    trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, 0, ent);

    return (trace.startsolid) ? sv.edicts : NULL;
}


/*
===============================================================================

LINE TESTING IN HULLS

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define DIST_EPSILON (0.03125f)

/*
==================
SV_RecursiveHullCheck

==================
*/
bool SV_RecursiveHullCheck(
    Hull_p hull, int    num,
    float  p1f, float  p2f,
    vec3_t p1, vec3_t p2,
    trace_p trace
) {
    // check for empty
    if (num < 0) {
        if (num != CONTENTS_SOLID) {
            trace->allsolid = false;
            if (num == CONTENTS_EMPTY)  trace->inopen = true;
            else                        trace->inwater = true;
        }
        else { trace->startsolid = true; }
        return true;  // empty
    }

    if ((num < hull->firstclipnode) ||
        (num > hull->lastclipnode)
        )
        Sys_Error("SV_RecursiveHullCheck: bad node number");

    //
    // find the point distances
    //
    dClipNode_p node = hull->clipnodes + num;
    mPlane_p plane = hull->planes + node->planenum;

    float t1, t2;
    if (plane->type < 3) {
        t1 = p1[plane->type] - plane->dist;
        t2 = p2[plane->type] - plane->dist;
    }
    else {
        t1 = DotProduct(plane->normal, p1) - plane->dist;
        t2 = DotProduct(plane->normal, p2) - plane->dist;
    }

#if 1
    if ((t1 >= 0) && (t2 >= 0))     return SV_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
    if ((t1 < 0) && (t2 < 0))       return SV_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);
#else
    if ((t1 >= DIST_EPSILON && t2 >= DIST_EPSILON) || (t2 > t1 && t1 >= 0))     return SV_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
    if ((t1 <= -DIST_EPSILON && t2 <= -DIST_EPSILON) || (t2 < t1 && t1 <= 0))   return SV_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);
#endif

    // put the crosspoint DIST_EPSILON pixels on the near side
    float  frac;
    if (t1 < 0)     frac = (t1 + DIST_EPSILON) / (t1 - t2);
    else            frac = (t1 - DIST_EPSILON) / (t1 - t2);
    CLAMP(0.0, frac, 1.0);

    float midf = p1f + (p2f - p1f) * frac;
    vec3_t  mid;
    for (int i = 0; i < VECT_DIM; i++)
        mid[i] = p1[i] + frac * (p2[i] - p1[i]);


    int side = (t1 < 0);

    // move up to the node
    if (!SV_RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid, trace))
        return false;

#ifdef PARANOID
    if (SV_HullPointContents(sv_hullmodel, mid, node->children[side])
        == CONTENTS_SOLID) {
        Con_Printf("mid PointInHullSolid\n");
        return false;
    }
#endif

    if (SV_HullPointContents(hull, node->children[side ^ 1], mid) != CONTENTS_SOLID)
        // go past the node
        return SV_RecursiveHullCheck(hull, node->children[side ^ 1], midf, p2f, mid, p2, trace);

    if (trace->allsolid)    return false;  // never got out of the solid area

    //==================
    // the other side of the node is solid, this is the impact point
    //==================
    if (!side) {
        VectorCopy(plane->normal, trace->plane.normal);
        trace->plane.dist = plane->dist;
    }
    else {
        VectorSubtract(vec3_origin, plane->normal, trace->plane.normal);
        trace->plane.dist = -plane->dist;
    }

    while (
        SV_HullPointContents(hull, hull->firstclipnode, mid) == CONTENTS_SOLID) { // shouldn't really happen, but does occasionally
        frac -= 0.1f;
        if (frac < 0) {
            trace->fraction = midf;
            VectorCopy(mid, trace->endpos);
            Con_DPrintf("backup past 0\n");
            return false;
        }
        midf = p1f + (p2f - p1f) * frac;
        for (int i = 0; i < VECT_DIM; i++) {
            mid[i] = p1[i] + frac * (p2[i] - p1[i]);
        }
    }

    trace->fraction = midf;
    VectorCopy(mid, trace->endpos);

    return false;
}


/*
==================
SV_ClipMoveToEntity

Handles selection or creation of a clipping hull, and offseting (and
eventually rotation) of the end points
==================
*/
trace_t SV_ClipMoveToEntity(edict_p ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end) {
    // fill in a default trace
    trace_t trace;  memset(&trace, 0, sizeof(trace_t));
    trace.fraction = 1;
    trace.allsolid = true;
    VectorCopy(end, trace.endpos);

    // get the clipping hull
    vec3_t offset;
    Hull_p hull = SV_HullForEntity(ent, mins, maxs, offset);

    vec3_t start_l; VectorSubtract(start, offset, start_l);
    vec3_t end_l; VectorSubtract(end, offset, end_l);

#ifdef QUAKE2
    // rotate start and end into the models frame of reference
    if ((solid_t)ent->v.solid == SOLID_BSP &&
        (
            ent->v.angles[0] ||
            ent->v.angles[1] ||
            ent->v.angles[2])
        ) {
        // vec3_t a;
        vec3_t forward, right, up; AngleVectors(ent->v.angles, forward, right, up);

        vec3_t temp; VectorCopy(start_l, temp);
        start_l[0] = DotProduct(temp, forward);
        start_l[1] = -DotProduct(temp, right);
        start_l[2] = DotProduct(temp, up);

        VectorCopy(end_l, temp);
        end_l[0] = DotProduct(temp, forward);
        end_l[1] = -DotProduct(temp, right);
        end_l[2] = DotProduct(temp, up);
    }
#endif

    // trace a line through the apropriate clipping hull
    SV_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, start_l, end_l, &trace);

#ifdef QUAKE2
    // rotate endpos back to world frame of reference
    if (((solid_t)ent->v.solid == SOLID_BSP) &&
        (
            ent->v.angles[0] ||
            ent->v.angles[1] ||
            ent->v.angles[2]) &&
        (trace.fraction != 1)) {
        vec3_t a; VectorSubtract(vec3_origin, ent->v.angles, a);
        vec3_t forward, right, up;  AngleVectors(a, forward, right, up);

        {
            vec3_t temp; VectorCopy(trace.endpos, temp);
            trace.endpos[0] = DotProduct(temp, forward);
            trace.endpos[1] = -DotProduct(temp, right);
            trace.endpos[2] = DotProduct(temp, up);
        }
        {
            vec3_t temp; VectorCopy(trace.plane.normal, temp);
            trace.plane.normal[0] = DotProduct(temp, forward);
            trace.plane.normal[1] = -DotProduct(temp, right);
            trace.plane.normal[2] = DotProduct(temp, up);
        }
    }
#endif

    // fix trace up by the offset
    if (trace.fraction != 1)
        VectorAdd(trace.endpos, offset, trace.endpos);

    // did we clip the move?
    if ((trace.fraction < 1) || trace.startsolid)
        trace.ent = ent;

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
            (touch == clip->passedict))
            continue;

        if (touch->v.solid == SOLID_TRIGGER)    Sys_Error("Trigger in clipping list");

        if ((clip->type == MOVE_NOMONSTERS) &&
            (touch->v.solid != SOLID_BSP))
            continue;

        if (
            (
                clip->boxmins[0] > touch->v.absmax[0] ||
                clip->boxmins[1] > touch->v.absmax[1] ||
                clip->boxmins[2] > touch->v.absmax[2]) ||
            (
                clip->boxmaxs[0] < touch->v.absmin[0] ||
                clip->boxmaxs[1] < touch->v.absmin[1] ||
                clip->boxmaxs[2] < touch->v.absmin[2]) ||
            (
                clip->passedict &&
                clip->passedict->v.size[0] &&
                !touch->v.size[0]
                )
            )
            continue; // points never interact

        // might intersect, so do an exact clip
        if (clip->trace.allsolid)   return;

        if (clip->passedict) {
            if ((PROG_TO_EDICT(touch->v.owner) == clip->passedict) || // don't clip against own missiles
                (PROG_TO_EDICT(clip->passedict->v.owner) == touch)) // don't clip against owner
                continue;
        }

        trace_t trace;
        if ((int)touch->v.flags & FL_MONSTER)       trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins2, clip->maxs2, clip->end);
        else                                        trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins, clip->maxs, clip->end);

        if (trace.allsolid || trace.startsolid ||
            trace.fraction < clip->trace.fraction) {
            trace.ent = touch;
            if (clip->trace.startsolid
                ) {
                clip->trace = trace;
                clip->trace.startsolid = true;
            }
            else clip->trace = trace;
        }
        else if (trace.startsolid)
            clip->trace.startsolid = true;
    }

    // recurse down both sides
    if (node->axis == -1)
        return;

    if (clip->boxmaxs[node->axis] > node->dist)     SV_ClipToLinks(node->children[0], clip);
    if (clip->boxmins[node->axis] < node->dist)     SV_ClipToLinks(node->children[1], clip);
}


/*
==================
SV_MoveBounds
==================
*/
void SV_MoveBounds(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs) {
#if 0
    // debug to test against everything
    boxmins[0] = boxmins[1] = boxmins[2] = -9999;
    boxmaxs[0] = boxmaxs[1] = boxmaxs[2] = 9999;
#else
    for (int i = 0; i < VECT_DIM; i++) {
        if (end[i] > start[i]) {
            boxmins[i] = start[i] + mins[i] - 1;
            boxmaxs[i] = end[i] + maxs[i] + 1;
        }
        else {
            boxmins[i] = end[i] + mins[i] - 1;
            boxmaxs[i] = start[i] + maxs[i] + 1;
        }
    }
#endif
}

/*
==================
SV_Move
==================
*/
trace_t SV_Move(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, phymovetype_t type, edict_p passedict) {
    moveClip_t clip;  memset(&clip, 0, sizeof(moveClip_t));

    // clip to world
    clip.trace = SV_ClipMoveToEntity(sv.edicts, start, mins, maxs, end);

    clip.start = start;
    clip.end = end;
    clip.mins = mins;
    clip.maxs = maxs;
    clip.type = type;
    clip.passedict = passedict;

    if (type == MOVE_MISSILE)
        for (int i = 0; i < VECT_DIM; i++) {
            clip.mins2[i] = -15;
            clip.maxs2[i] = 15;
        }
    else {
        VectorCopy(mins, clip.mins2);
        VectorCopy(maxs, clip.maxs2);
    }

    // create the bounding box of the entire move
    SV_MoveBounds(start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs);

    // clip to entities
    SV_ClipToLinks(_sv_AreaNodes, &clip);

    return clip.trace;
}

/*
==================
BOPS_Error

Split out like this for ASM to call.
==================
*/
void BOPS_Error() { Sys_Error("BoxOnPlaneSide:  Bad signbits"); }


#if !id386




/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, mPlane_p p) {
#if 0 // this is done by the BOX_ON_PLANE_SIDE macro before calling this
    // function
// fast axial cases
    if (p->type < 3) {
        if (p->dist <= emins[p->type])  return 1;
        if (p->dist >= emaxs[p->type])  return 2;
        return 3;
    }
#endif

    // general case
    float dist1, dist2;
    switch (p->signbits) {
    case 0:
        dist1 =
            p->normal[0] * emaxs[0] +
            p->normal[1] * emaxs[1] +
            p->normal[2] * emaxs[2];
        dist2 =
            p->normal[0] * emins[0] +
            p->normal[1] * emins[1] +
            p->normal[2] * emins[2];
        break;
    case 1:
        dist1 =
            p->normal[0] * emins[0] +
            p->normal[1] * emaxs[1] +
            p->normal[2] * emaxs[2];
        dist2 =
            p->normal[0] * emaxs[0] +
            p->normal[1] * emins[1] +
            p->normal[2] * emins[2];
        break;
    case 2:
        dist1 =
            p->normal[0] * emaxs[0] +
            p->normal[1] * emins[1] +
            p->normal[2] * emaxs[2];
        dist2 =
            p->normal[0] * emins[0] +
            p->normal[1] * emaxs[1] +
            p->normal[2] * emins[2];
        break;
    case 3:
        dist1 =
            p->normal[0] * emins[0] +
            p->normal[1] * emins[1] +
            p->normal[2] * emaxs[2];
        dist2 =
            p->normal[0] * emaxs[0] +
            p->normal[1] * emaxs[1] +
            p->normal[2] * emins[2];
        break;
    case 4:
        dist1 =
            p->normal[0] * emaxs[0] +
            p->normal[1] * emaxs[1] +
            p->normal[2] * emins[2];
        dist2 =
            p->normal[0] * emins[0] +
            p->normal[1] * emins[1] +
            p->normal[2] * emaxs[2];
        break;
    case 5:
        dist1 =
            p->normal[0] * emins[0] +
            p->normal[1] * emaxs[1] +
            p->normal[2] * emins[2];
        dist2 =
            p->normal[0] * emaxs[0] +
            p->normal[1] * emins[1] +
            p->normal[2] * emaxs[2];
        break;
    case 6:
        dist1 =
            p->normal[0] * emaxs[0] +
            p->normal[1] * emins[1] +
            p->normal[2] * emins[2];
        dist2 =
            p->normal[0] * emins[0] +
            p->normal[1] * emaxs[1] +
            p->normal[2] * emaxs[2];
        break;
    case 7:
        dist1 =
            p->normal[0] * emins[0] +
            p->normal[1] * emins[1] +
            p->normal[2] * emins[2];
        dist2 =
            p->normal[0] * emaxs[0] +
            p->normal[1] * emaxs[1] +
            p->normal[2] * emaxs[2];
        break;
    default:
        dist1 = dist2 = 0;  // shut up compiler
        BOPS_Error();
        break;
    }

#if 0
    vec3_t corners[2];
    for (int i = 0; i < VECT_DIM; i++) {
        if (plane->normal[i] < 0) {
            corners[0][i] = emins[i];
            corners[1][i] = emaxs[i];
        }
        else {
            corners[1][i] = emins[i];
            corners[0][i] = emaxs[i];
        }
    }
    dist = DotProduct(plane->normal, corners[0]) - plane->dist;
    dist2 = DotProduct(plane->normal, corners[1]) - plane->dist;
    sides = 0;
    if (dist1 >= 0)     sides = 1;
    if (dist2 < 0)      sides |= 2;

#endif

    int sides = 0;
    if (dist1 >= p->dist)   sides |= 1;
    if (dist2 < p->dist)    sides |= 2;

#ifdef PARANOID
    if (sides == 0) Sys_Error("BoxOnPlaneSide: sides==0");
#endif

    return sides;
}

#endif
