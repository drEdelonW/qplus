#pragma once
#include "vector.h"

#ifndef M_PI
# define M_PI  (3.14159265358979323846) /* matches value in gcc v2 math.h */
#endif
#define DEG2RAD(a) (a * M_PI) / 180.0F
#define RAD2DEG(a) (a / M_PI) * 180.0F
#ifdef __cplusplus
extern "C" {
#endif

    float   anglemod(float a);
    float   angledelta(float a);
    vec3_t  VectorAngleProc(vec3_t angles);

#ifdef __cplusplus
}
#endif