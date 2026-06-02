#pragma once

#include "types.h"
#include "vector.h"
#include "assert.h"

typedef struct {
    vec3_t  origin;
    float   radius;
    LegacyTimeDelta_t   die;        // stop lighting after this time
    LegacyTimeDelta_t   decay;      // drop this each second
    float   minlight;   // don't add when contributing less
    int32_t key;
#ifdef QUAKE2
    bool dark;   // subtracts light instead of adding
#endif
} dLight_t;
typedef dLight_t* dLight_p;
STATIC_ASSERT_SIZE(dLight_t, 32); // QUAKE2 not handled

extern dLight_t     cl_dlights[MAX_DLIGHTS];
