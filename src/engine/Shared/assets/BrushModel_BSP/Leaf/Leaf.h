#pragma once

#include "types.h"
#include "Node.h"
#include "Surface.h"
#include "Sound_struct.h"
#include "eFrag.h"

#define MAX_MAP_LEAFS           (8192)

// it was [mleaf_t]
typedef struct mLeaf_s {
    // common with node
    int32_t contents;   // wil be a negative contents number
    int32_t visframe;   // node needs to be traversed if current
    int16_t minmaxs[6]; // for bounding box culling
    mNode_p parent;

    // leaf specific
    uint8_p compressed_vis;
    efrag_p efrags;
    mSurface_ar firstmarksurface;
    int32_t nummarksurfaces;
    int32_t key;        // BSP sequence number for leaf's contents
    uint8_t ambient_sound_level[NUM_AMBIENTS];
} mLeaf_t;
typedef mLeaf_t* mLeaf_p;


// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct {
    int32_t contents;
    int32_t visofs;    // -1 = no visibility info

    int16_t mins[3];   // for frustum culling
    int16_t maxs[3];

    uint16_t firstmarksurface;
    uint16_t nummarksurfaces;

    uint8_t ambient_level[NUM_AMBIENTS];
} dLeaf_t;
typedef dLeaf_t* dLeaf_p;
