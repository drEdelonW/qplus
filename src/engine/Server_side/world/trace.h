#pragma once
#include "vector.h"
#include "types.h"
#include "Edict.h"
#include "Hull.h"

typedef struct {
    vec3_t  normal;
    float   dist;
} Plane_t;

typedef struct {    // sequence like in [pr_global_struct->trace_allsolid]
    bool    allsolid;   // if true, plane is not valid
    bool    startsolid; // if true, the initial point was in a solid area
    float   fraction;   // time completed, 1.0 = didn't hit anything
    vec3_t  endpos;     // final position
    Plane_t plane;      // surface normal at impact
    edict_p pEnt;       // entity the surface is on
    bool    inopen;
    bool    inwater;
} trace_t;
typedef trace_t* trace_p;

bool SV_RecursiveHullCheck(Hull_p hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_p trace);

#include "PhyMoveType.h"
trace_t SV_Move(vec3_t start, BBox_t m, vec3_t end, phymovetype_t type, edict_p passedict); 
// if the entire move stays in a solid volume, trace.allsolid will be set
// if the starting point is in a solid, it will be allowed to move out to an open area
// nomonsters is used for line of sight or edge testing, where mosnters shouldn't be considered solid objects
// passedict is explicitly excluded from clipping checks (normally NULL)
