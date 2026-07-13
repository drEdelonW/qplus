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
    entities never clip against themselves, or their owner
    line of sight checks trace->crosscontent, but bullets don't
*/


/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/

areaNode_t _sv_AreaNodes[AREA_NODES];
static int _sv_NumAreaNodes;


#define AREA_DEPTH 4
areaNode_p SV_CreateAreaNode(int depth, BBox_t bb) {
    areaNode_p anode = &_sv_AreaNodes[_sv_NumAreaNodes];
    _sv_NumAreaNodes++;

    ClearLink(&anode->trigger_edicts);
    ClearLink(&anode->solid_edicts);

    if (depth == AREA_DEPTH) {
        anode->axis = AXIS_LEAF;
        anode->children[0] = anode->children[1] = NULL;
        return anode;
    }

    vec3_t size = BBoxSize(bb);
    anode->axis = (size.x > size.y) ?
        AXIS_X : AXIS_Y;

    anode->dist = 0.5f * (bb.maxs.v[anode->axis] + bb.mins.v[anode->axis]);
    BBox_t m1 = bb;
    BBox_t m2 = bb;
    m1.maxs.v[anode->axis] = m2.mins.v[anode->axis] = anode->dist;

    anode->children[0] = SV_CreateAreaNode(depth + 1, m2);
    anode->children[1] = SV_CreateAreaNode(depth + 1, m1);

    return anode;
}


void SV_ClearWorld() {
    SV_InitBoxHull();

    memset(_sv_AreaNodes, 0, sizeof(_sv_AreaNodes));
    _sv_NumAreaNodes = 0;
    SV_CreateAreaNode(0, sv.worldmodel->BB);
}


void SV_UnlinkEdict(edict_p ent) {
    if (!ent->area.prev)    return;  // not linked in anywhere

    RemoveLink(&ent->area);
    ent->area.prev = ent->area.next = NULL;
}



#include "BBox_tools.h"
void SV_FindTouchedLeafs(edict_p ent, mNode_p node) {
    if (node->contents == CONTENTS_SOLID)       return;

    // add an efrag if the node is a leaf
    if (node->contents < CONTENTS_NODE) {
        if (ent->num_leafs == EntLeafsMax)      return;

        mLeaf_p leaf = (mLeaf_p)node;
        int32_t leafnum = leaf - sv.worldmodel->leafs - 1; // TODO: typedef index of leaf

        ent->leafnums[ent->num_leafs] = (int16_t)leafnum;
        ent->num_leafs++;
        return;
    }

    // NODE_MIXED
    BBox_t _bb = EvAbsBBox(&ent->v);
    PlaneSide_t sides = BOX_ON_PLANE_SIDE(_bb, node->plane);

    // recurse down the contacted sides
    if (sides & PsFront)    SV_FindTouchedLeafs(ent, node->children[0]);
    if (sides & PsBack)     SV_FindTouchedLeafs(ent, node->children[1]);
}



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

#if 0
        if ((ent->v.absmin.x > touch->v.absmax.x) ||
            (ent->v.absmin.y > touch->v.absmax.y) ||
            (ent->v.absmin.z > touch->v.absmax.z) ||
            (ent->v.absmax.x < touch->v.absmin.x) ||
            (ent->v.absmax.y < touch->v.absmin.y) ||
            (ent->v.absmax.z < touch->v.absmin.z)
            )   continue;
#else
        if (!BBoxOverlaps(
            EvAbsBBox(&ent->v),
            EvAbsBBox(&touch->v))
            )   continue;
#endif

        int old_self = pr_global_struct->self;
        int old_other = pr_global_struct->other;

        pr_global_struct->self = ED_GetEDictOffs(touch);
        pr_global_struct->other = ED_GetEDictOffs(ent);
        pr_global_struct->time = (float)SV_GetTime();
        PR_ExecuteProgram(touch->v.touch);

        pr_global_struct->self = old_self;
        pr_global_struct->other = old_other;
    }

    // recurse down both sides
    if (node->axis == AXIS_LEAF)       return;

    if (ent->v.absmax.v[node->axis] > node->dist)     SV_TouchLinks(ent, node->children[0]);
    if (ent->v.absmin.v[node->axis] < node->dist)     SV_TouchLinks(ent, node->children[1]);
}



void SV_LinkEdict(edict_p ent, bool touch_triggers) {
    if (ent->area.prev)
        SV_UnlinkEdict(ent); // unlink from old position

    if ((ent == Edicts) ||  // don't add the world
        (ent->free)
        )   return;
    // set the abs box

#ifdef QUAKE2
    if (ent->v.solid == SOLID_BSP &&
        (
            ent->v.angles.pitch ||
            ent->v.angles.yaw ||
            ent->v.angles.roll
            )) { // expand for rotation
        float max = 0;
        for (int i = 0; i < VECT_DIM; i++) {
            float v = fabs(ent->v.mins[i]);
            ClampLessThen(&max, v);
            v = fabs(ent->v.maxs[i]);
            ClampLessThen(&max, v);
        }

        ent->v.absmin = VectorSubtract(ent->v.origin, Scalar2Vector(max));
        ent->v.absmax = VectorAdd(ent->v.origin, Scalar2Vector(max));

    }
    else
#endif
    {
        ent->v.absmin = VectorAdd(ent->v.origin, ent->v.mins);
        ent->v.absmax = VectorAdd(ent->v.origin, ent->v.maxs);
    }

    //
    // to make items easier to pick up and allow them to be grabbed off
    // of shelves, the abs sizes are expanded
    //
    if ((int)ent->v.flags & FL_ITEM) {
        ent->v.absmin.x -= 15;
        ent->v.absmin.y -= 15;

        ent->v.absmax.x += 15;
        ent->v.absmax.y += 15;
    }
    else { // because movement is clipped an epsilon away from an actual edge,
        // we must fully check even when bounding boxes don't quite touch
        ent->v.absmin.x -= 1;
        ent->v.absmin.y -= 1;
        ent->v.absmin.z -= 1;

        ent->v.absmax.x += 1;
        ent->v.absmax.y += 1;
        ent->v.absmax.z += 1;
    }

    // link to PVS leafs
    ent->num_leafs = EntLeafsFirst;
    if (ent->v.modelindex)
        SV_FindTouchedLeafs(ent, sv.worldmodel->nodes);

    if (ent->v.solid == SOLID_NOT)      return;

    // find the first node that the ent's box crosses
    areaNode_p node = _sv_AreaNodes;
    while (1) {
        if (node->axis == AXIS_LEAF)       break;

        /**/ if (ent->v.absmin.v[node->axis] > node->dist)      node = node->children[0];
        else if (ent->v.absmax.v[node->axis] < node->dist)      node = node->children[1];
        else    break;  // crosses the node
    }

    // link it in

    if (ent->v.solid == SOLID_TRIGGER)  InsertLinkBefore(&ent->area, &node->trigger_edicts);
    else                                InsertLinkBefore(&ent->area, &node->solid_edicts);

    // if touch_triggers, touch all entities at this node and decend for more
    if (touch_triggers)     SV_TouchLinks(ent, _sv_AreaNodes);
}
