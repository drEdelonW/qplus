#pragma once
#include "vector.h"
#include "angles_indices.h"

#ifndef M_PI
#define M_PI  (3.14159265358979323846) /* matches value in gcc v2 math.h */
#endif
#define DEG2RAD(a) (a * M_PI) / 180.0F

void    AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
float   anglemod(float a);
