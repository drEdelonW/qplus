#include "angle.h"

float anglemod(float a) {
#if 0
    if (a >= 0) a -= 360 * (int)(a / 360);
    else        a += 360 * (1 + (int)(-a / 360));
#endif
    a = (360.0f / (0xFFFF + 1)) * ((int)(a * ((0xFFFF + 1) / 360.0f)) & 0xFFFF);
    return a;
}

float angledelta(float a) {
    a = anglemod(a);
    if (a > 180)
        a -= 360;
    return a;
}

vec3_t VectorAngleProc(vec3_t angles) {
    vec3_t out = angles;
    for (int j = 0; j < VECT_DIM; j++) {
        /* */if (out.v[j] > 180.0f)    out.v[j] -= 360.0f;
        else if (out.v[j] < -180.0f)   out.v[j] += 360.0f;
    }
    return out;
}