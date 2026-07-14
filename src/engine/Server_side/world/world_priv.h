#pragma once

#include "world.h"
#include "bspfile.h"
#include <string.h>
#include "server.h"
#include "console.h"
#include "q_tools.h"
#include "mathlib.h"
#include "progs.h"
#include "host.h"
#include "GlobVars.h"
#include "BBox.h"
#include "trace.h"

/*
=============================================================================
world_priv.h - private declarations for world subsystem
=============================================================================
*/
#pragma once

// moveClip_t - used by world_move.c and world_area.c
typedef struct {
    BBox_t          box;
    BBox_t          mv;
    BBox_t          m2;
    vec3_t          start;
    vec3_t          end;
    trace_t         trace;
    phymovetype_t   moveType;
    edict_p         passedict;
} moveClip_t;
typedef moveClip_t* moveClip_p;

// areaNode_t - used by world_area.c and world_move.c
typedef enum {
    AXIS_LEAF = -1,
    AXIS_X    =  0,
    AXIS_Y    =  1,
} AreaAxis_t;

typedef struct areaNode_s areaNode_t;
typedef areaNode_t* areaNode_p;
struct areaNode_s {
    AreaAxis_t  axis;
    float       dist;
    areaNode_p  children[2];
    link_t      trigger_edicts;
    link_t      solid_edicts;
};

#define AREA_NODES 32

extern areaNode_t   _sv_AreaNodes[AREA_NODES];

// world_hull.c
// Hull_p      SV_HullForBox(BBox_t bb);
Hull_p      SV_HullForEntity(edict_p ent, BBox_t bb, vec3_p offset);
// contents_t  SV_HullPointContents(Hull_p hull, int num, vec3_t point);
void        SV_InitBoxHull(void);

// world_area.c
// areaNode_p  SV_CreateAreaNode(int depth, BBox_t bb);
// void        SV_FindTouchedLeafs(edict_p ent, mNode_p node);
// void        SV_TouchLinks(edict_p ent, areaNode_p node);

// world_move.c
// trace_t SV_ClipMoveToEntity(edict_p ent, vec3_t start, BBox_t bb, vec3_t end);
// void    SV_ClipToLinks(areaNode_p node, moveClip_p clip);
// void    SV_MoveBounds(vec3_t start, BBox_t bb, vec3_t end, BBox_p box);