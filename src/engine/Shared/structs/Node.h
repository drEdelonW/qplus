#pragma once

#include "types.h"
#include "Plane.h"
#include "BBox.h"

typedef enum {
    CONTENTS_NODE         =  (int16_t)0,
  /* vvv[Leaf]vvv ^^^[Node]^^^ */
    CONTENTS_EMPTY        = -1,  // used by watertype
    CONTENTS_SOLID        = -2,
    CONTENTS_WATER        = -3,  // used by watertype
    CONTENTS_SLIME        = -4,  // used by watertype
    CONTENTS_LAVA         = -5,  // used by watertype
    CONTENTS_SKY          = -6,
    CONTENTS_ORIGIN       = -7,  // removed at CSG time
    CONTENTS_CLIP         = -8,  // changed to CONTENTS_SOLID

    CONTENTS_CURRENT_0    = -9,
    CONTENTS_CURRENT_90   = -10,
    CONTENTS_CURRENT_180  = -11,
    CONTENTS_CURRENT_270  = -12,
    CONTENTS_CURRENT_UP   = -13,
    CONTENTS_CURRENT_DOWN = -14
} contents_t;   // should be int16_t


typedef struct mNode_s mNode_t;
typedef mNode_t* mNode_p;
struct mNode_s {    // TODO: merge in shared head structure with  mLeaf_s
    // common with leaf
    contents_t  contents;  // 0, to differentiate from leafs
    int32_t     visframe;  // node needs to be traversed if current
    BBox_t      bb;
    mNode_p     parent;

    // node specific
    mPlane_p    plane;
    mNode_p     children[PlaneSides];
    uint16_t    firstsurface;
    uint16_t    numsurfaces;
};

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    int32_t     planenum;
    int16_t     children[PlaneSides]; // [contents_t] negative numbers are -(leafs+1), not nodes

    int16_t     mins[VECT_DIM];  // for sphere culling
    int16_t     maxs[VECT_DIM];

    uint16_t    firstface;
    uint16_t    numfaces; // counting both sides
} dNode_t;      STATIC_ASSERT_SIZE(dNode_t, 4 + 2*2 + 2*2*3 + 2*2); // 24
typedef dNode_t* dNode_p;


typedef enum {
    isNode, // >= CONTENTS_NODE
    isLeaf  // < CONTENTS_NODE
} nodeKind_t;

#include "Node.h"
static inline nodeKind_t NodeKind(mNode_p pNode) {
    return (pNode->contents < CONTENTS_NODE)?
        isLeaf : isNode;
}
