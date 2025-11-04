#pragma once

#include "vector.h"
typedef struct {
    vec3_t viewangles;
    // intended velocities
    float forwardmove;
    float sidemove;
    float upmove;
#ifdef QUAKE2
    uint8_t lightlevel;
#endif
} UserCmd_t;
typedef UserCmd_t* UserCmd_p;