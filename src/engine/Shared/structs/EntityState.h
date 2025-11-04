#pragma once

#include "vector.h"
#include "types.h"
typedef struct {
    vec3_t  origin;
    vec3_t  angles;
    int32_t modelindex;
    int32_t frame;
    int32_t colormap;
    int32_t skin;
    int32_t effects;
} EntityState_t;