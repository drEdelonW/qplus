#pragma once

#include "types.h"

typedef struct {
    int32_t planenum;
    int16_t children[2]; // negative numbers are contents
} dClipNode_t;      STATIC_ASSERT_SIZE(dClipNode_t, 4 + 2*2); // 8
typedef dClipNode_t* dClipNode_p;
