#pragma once

#include "vector.h"
#include "angle.h"
typedef struct {
#if 0   // TODO: not used?
    ang3_t viewangles;
#endif
    vec3_t move;    // intended velocities
#ifdef QUAKE2
    uint8_t lightlevel;
#endif
} UserCmd_t;
typedef UserCmd_t* UserCmd_p;