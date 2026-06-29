#include "angle.h"
#include "fixed.h"

float anglemod(float a) {
#if 0
    if (a >= 0) a -= 360 * (int)(a / 360);
    else        a += 360 * (1 + (int)(-a / 360));
#endif
    return
        (360.0f / FIXED16_ONE) *
        FIXED16_FRAC((fixed16_t)(a * (FIXED16_ONE / 360.0f)));
}

float angledelta(float a) {
    a = anglemod(a);
    if (a > 180.0f)     a -= 360.0f;
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