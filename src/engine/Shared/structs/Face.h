#pragma once
#include "types.h"
#include "assert.h"
#include "enginedefs.h"

typedef struct {
    int16_t planenum;
    int16_t side;

    int32_t firstedge;  // we must support > 64k edges scope
    uint16_t numedges;  // up to 65k edges

    int16_t texinfo;

    uint8_t styles[MAXLIGHTMAPS];    // lighting info
    int32_t lightofs;   // start of [numstyles*surfsize] samples
} dFace_t;          STATIC_ASSERT_SIZE(dFace_t, 2*2 + 4 + 2*2 + 1*4 + 4); // 20
typedef dFace_t* dFace_p;
