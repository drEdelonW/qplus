#pragma once

#include "types.h"

typedef struct dTriangle_s {
    int32_t facesfront;
    int32_t vertindex[3];
} dTriangle_t;
typedef dTriangle_t* dTriangle_p;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mTriangle_s {
    int32_t facesfront;
    int32_t vertindex[3];
} mTriangle_t;
typedef mTriangle_t* mTriangle_p;

// This mirrors trivert_t in trilib.h, is present so Quake knows how to
// load this data

typedef struct {
    uint8_t v[3];
    uint8_t lightnormalindex;
} TriVertx_t;
typedef TriVertx_t* TriVertx_p;
