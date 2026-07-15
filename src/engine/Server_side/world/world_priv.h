#pragma once

#include "world.h"
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
Hull_p  SV_HullForEntity(edict_p ent, BBox_t bb, vec3_p offset);
void    SV_InitBoxHull(void);

