#pragma once
#include "types.h"
#include "assert.h"
#include "enginedefs.h"

typedef struct {
    int16_t planenum;
    int16_t side;

    int32_t firstedge;  // we must support > 64k edges
    int16_t numedges;
    int16_t texinfo;

    // lighting info
    uint8_t styles[MAXLIGHTMAPS];
    int32_t lightofs;   // start of [numstyles*surfsize] samples
} dFace_t;
typedef dFace_t* dFace_p;
STATIC_ASSERT_SIZE(dFace_t, 2*2 + 4 + 2*2 + 1*4 + 4); // 20
