#pragma once

#include "assert.h"
#include "types.h"
#include "BBox.h"
#include "Leaf_pre.h"
#include "Node.h"
#include "Surface_pre.h"
#include "structs/Sound_struct.h" // TODO: fix name and kind of content
#include "eFrag_pre.h"

#define MAX_MAP_LEAFS   (8192) /* 8k */

// it was [mleaf_t]
struct mLeaf_s {    // TODO: merge in shared head structure with  mNode_s
    // common with node
    contents_t  contents;   // wil be a negative contents number
    int32_t     visframe;   // node needs to be traversed if current
    BBox_t      bb;
    mNode_p     parent;

    // leaf specific
    uint8_p compressed_vis;
    efrag_p efrags;
    mSurface_ar firstmarksurface;
    int32_t nummarksurfaces;
    int32_t key;        // BSP sequence number for leaf's contents
    uint8_t ambient_sound_level[NUM_AMBIENTS];
};



// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct {
    int32_t contents;
    int32_t visofs;    // -1 = no visibility info

#if 1 // TODO: wrap it to vec3i and BBoxi or some like this
    int16_t mins[3];   // for frustum culling
    int16_t maxs[3];   // for frustum culling
#else
    BBox_t bb;
#endif

    uint16_t firstmarksurface;
    uint16_t nummarksurfaces;

    uint8_t ambient_level[NUM_AMBIENTS];
} dLeaf_t;          STATIC_ASSERT_SIZE(dLeaf_t, 4*2 + 2*3*2 + 2*2 + 4); // 28
typedef dLeaf_t* dLeaf_p;

