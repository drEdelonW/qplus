#include "angle.h"
#include <math.h>

void AngleVectors(vec3_t angles, vec3_p forward, vec3_p right, vec3_p up) {
    float  angle;

    angle = angles.v[YAW] * (M_PI * 2 / 360);
    float sy = sin(angle);
    float cy = cos(angle);
    angle = angles.v[PITCH] * (M_PI * 2 / 360);
    float sp = sin(angle);
    float cp = cos(angle);
    angle = angles.v[ROLL] * (M_PI * 2 / 360);
    float sr = sin(angle);
    float cr = cos(angle);

    forward->v[0] = cp * cy;
    forward->v[1] = cp * sy;
    forward->v[2] = -sp;
    right->v[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
    right->v[1] = (-1 * sr * sp * sy + -1 * cr * cy);
    right->v[2] = -1 * sr * cp;
    up->v[0] = (cr * sp * cy + -sr * -sy);
    up->v[1] = (cr * sp * sy + -sr * cy);
    up->v[2] = cr * cp;
}

float anglemod(float a) {
#if 0
    if (a >= 0) a -= 360 * (int)(a / 360);
    else        a += 360 * (1 + (int)(-a / 360));
#endif
    a = (360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535);
    return a;
}
