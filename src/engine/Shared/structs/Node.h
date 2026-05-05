#pragma once

#include "types.h"
#include "Plane.h"

struct mNode_s;
typedef struct mNode_s mNode_t;
typedef mNode_t* mNode_p;
struct mNode_s {
    // common with leaf
    int32_t     contents;  // 0, to differentiate from leafs
    int32_t     visframe;  // node needs to be traversed if current
#ifdef GLQUAKE
    float       minmaxs[6];  // for bounding box culling
#else
    int16_t     minmaxs[6];  // for bounding box culling
#endif
    mNode_p     parent;
    // node specific
    mPlane_p    plane;
    mNode_p     children[2];
    uint16_t    firstsurface;
    uint16_t    numsurfaces;
};

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    int32_t     planenum;
    int16_t     children[2]; // negative numbers are -(leafs+1), not nodes
    int16_t     mins[3];  // for sphere culling
    int16_t     maxs[3];
    uint16_t    firstface;
    uint16_t    numfaces; // counting both sides
} dNode_t;
typedef dNode_t* dNode_p;