#pragma once

#include "vector.h"
#include "angle.h"
typedef struct {
    // ang3_t viewangles; // TODO: not used?
    // intended velocities
#if 0
    float forwardmove;
    float sidemove;
    float upmove;
#else
    vec3_t move;
#endif
#ifdef QUAKE2
    uint8_t lightlevel;
#endif
} UserCmd_t;
typedef UserCmd_t* UserCmd_p;