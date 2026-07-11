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

        _boxPlanes[i].type = HALF(i);
        _boxPlanes[i].normal.v[HALF(i)] = 1;
    }

}


/*
===================
SV_HullForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
Hull_p SV_HullForBox(BBox_t bb) {
    _boxPlanes[0].dist = bb.maxs.x;
    _boxPlanes[1].dist = bb.mins.x;
    _boxPlanes[2].dist = bb.maxs.y;
    _boxPlanes[3].dist = bb.mins.y;
    _boxPlanes[4].dist = bb.maxs.z;
    _boxPlanes[5].dist = bb.mins.z;
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
Hull_p SV_HullForEntity(edict_p ent, BBox_t bb, vec3_p offset) {
    Hull_p  hull;
    // decide which clipping hull to use, based on the size
    if (ent->v.solid == SOLID_BSP) { // explicit hulls in the BSP model
        if (ent->v.movetype != MOVETYPE_PUSH)
            Host_SysError("SOLID_BSP without MOVETYPE_PUSH");

        Model_p model = sv.models[(int)ent->v.modelindex];

        if (!model ||
            (model->type != mod_brush)
            )   Host_SysError("MOVETYPE_PUSH with a non bsp model");

        vec3_t size = VectorSubtract(bb.maxs, bb.mins);
        /**/ if (size.x < 3.0f)     hull = &model->hulls[0];
        else if (size.x <= 32.0f)   hull = &model->hulls[1];
        else                        hull = &model->hulls[2];

        // calculate an offset value to center the origin
        *offset = VectorAdd(VectorSubtract(hull->clip.mins, bb.mins), ent->v.origin);
    }
    else { // create a temp hull from bounding box sizes

        hull = SV_HullForBox(BBoxFromVec3(
            VectorSubtract(ent->v.mins, bb.maxs),
            VectorSubtract(ent->v.maxs, bb.mins)
        ));

        *offset = ent->v.origin;
    }

    return hull;
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
contents_t SV_HullPointContents(Hull_p hull, int num, vec3_t point) {
    while (num >= 0) {
        if ((num < hull->firstclipnode) ||
            (num > hull->lastclipnode)
            )   Host_SysError("SV_HullPointContents: bad node number");

        dClipNode_p node = hull->clipnodes + num;
        mPlane_p plane = hull->planes + node->planenum;

        float d =
            ((plane->type < 3) ?
                point.v[plane->type] : DotProduct(plane->normal, point)
                ) -
            plane->dist;

        num = node->children[(d < 0.f) ? 1 : 0];
    }

    return num;
}

#endif // !id386


/*
==================
SV_PointContents

==================
*/
contents_t SV_PointContents(vec3_t point) {
    contents_t cont = SV_HullPointContents(&sv.worldmodel->hulls[0], 0, point);
    if ((cont <= CONTENTS_CURRENT_0) &&
        (cont >= CONTENTS_CURRENT_DOWN)
        )   cont = CONTENTS_WATER;
    return cont;
}

contents_t SV_TruePointContents(vec3_t point) {
    return SV_HullPointContents(&sv.worldmodel->hulls[0], 0, point);
}

//===========================================================================



/*
===============================================================================

LINE TESTING IN HULLS

===============================================================================
*/

/*
==================
SV_RecursiveHullCheck

==================
*/
bool SV_RecursiveHullCheck(
    Hull_p hull, int  num,
    float  p1f, float p2f,
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
        Host_SysError("SV_RecursiveHullCheck: bad node number");

    //
    // find the point distances
    //
    dClipNode_p node = hull->clipnodes + num;
    mPlane_p plane = hull->planes + node->planenum;

    float t1, t2;
    if (plane->type < 3) {
        t1 = p1.v[plane->type] - plane->dist;
        t2 = p2.v[plane->type] - plane->dist;
    }
    else {
        t1 = DotProduct(plane->normal, p1) - plane->dist;
        t2 = DotProduct(plane->normal, p2) - plane->dist;
    }

#if 1
    if ((t1 >= 0.f) && (t2 >= 0.f))   return SV_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
    if ((t1 < 0.f) && (t2 < 0.f))     return SV_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);
#else
    if (((t1 >= DIST_EPSILON) && (t2 >= DIST_EPSILON)) || ((t2 > t1) && (t1 >= 0)))     return SV_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
    if (((t1 <= -DIST_EPSILON) && (t2 <= -DIST_EPSILON)) || ((t2 < t1) && (t1 <= 0)))   return SV_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);
#endif

    // put the crosspoint DIST_EPSILON pixels on the near side
    float  frac =
        (t1 + ((t1 < 0.f) ? DIST_EPSILON : -DIST_EPSILON)) /
        (t1 - t2);
    ClampInRange(0.f, &frac, 1.f);

    float midf = p1f + (p2f - p1f) * frac;

    vec3_t mid = VectorMA(p1, frac, VectorSubtract(p2, p1));

    bool side = (t1 < 0);

    // move up to the node
    if (!SV_RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid, trace))
        return false;

#ifdef PARANOID
    if (SV_HullPointContents(sv_hullmodel, mid, node->children[side])
        == CONTENTS_SOLID) {
        Con_Printf("mid PointInHullSolid\n");        return false;
    }
#endif

    if (SV_HullPointContents(hull, node->children[side ^ 1], mid) != CONTENTS_SOLID)    // go past the node
        return SV_RecursiveHullCheck(hull, node->children[side ^ 1], midf, p2f, mid, p2, trace);

    if (trace->allsolid)    return false;  // never got out of the solid area

    //==================
    // the other side of the node is solid, this is the impact point
    //==================
    if (!side) {
        trace->plane.normal = plane->normal;
        trace->plane.dist = plane->dist;
    }
    else {
        trace->plane.normal = VectorSubtract(v3Zero, plane->normal);
        trace->plane.dist = -plane->dist;
    }

    while (
        SV_HullPointContents(hull, hull->firstclipnode, mid) == CONTENTS_SOLID) { // shouldn't really happen, but does occasionally
        frac -= 0.1f;
        if (frac < 0) {
            trace->fraction = midf;
            trace->endpos = mid;
            Con_DPrintf("backup past 0\n");
            return false;
        }
        midf = p1f + (p2f - p1f) * frac;
        mid = VectorMA(p1, frac, VectorSubtract(p2, p1));
    }

    trace->fraction = midf;
    trace->endpos = mid;

    return false;
}
