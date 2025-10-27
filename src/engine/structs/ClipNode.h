#pragma once

#include "types.h"

typedef struct {
    int32_t planenum;
    int16_t children[2]; // negative numbers are contents
} dClipNode_t;
typedef dClipNode_t* dClipNode_p;