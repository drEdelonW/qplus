#pragma once

#include "types.h"

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct {
    uint16_t    v[2];  // Vertex numbers
} dEdge_t;
typedef dEdge_t* dEdge_p;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
    uint16_t v[2];
    uint32_t cachededgeoffset;
} mEdge_t;
typedef mEdge_t* mEdge_p;

//===========================================================================
// clipped bmodel edges

typedef struct bEdge_s bEdge_t;
typedef bEdge_t* bEdge_p;
typedef struct bEdge_s {
    mVertex_p v[2];
    bEdge_p pnext;
} bEdge_t;
