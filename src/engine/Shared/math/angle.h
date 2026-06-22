#pragma once
#include "vector.h"

#ifndef M_PI
# define M_PI  (3.14159265358979323846) /* matches value in gcc v2 math.h */
#endif
#define DEG2RAD(a) (a * M_PI) / 180.0F

float   anglemod(float a);
