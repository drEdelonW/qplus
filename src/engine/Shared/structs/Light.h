#pragma once

#include "types.h"
#include "vector.h"
#include "qTime.h"
#include "enginedefs.h"
#include "assert.h"

#define MAX_DLIGHTS             32

typedef struct {
    vec3_t  origin;
    float   radius;
    LegDt_t die;        // stop lighting after this time
    LegDt_t decay;      // drop this each second
    float   minlight;   // don't add when contributing less
    int32_t key;
#ifdef QUAKE2
    bool dark;   // subtracts light instead of adding
#endif
} dLight_t;
typedef dLight_t* dLight_p;
STATIC_ASSERT_SIZE(dLight_t, 32); // QUAKE2 not handled

extern dLight_t     cl_dlights[MAX_DLIGHTS];

#ifdef __cplusplus
extern "C" {
#endif

    dLight_p CL_AllocDlight(int32_t key);
    void CL_DecayLights();

#ifdef __cplusplus
}
#endif
