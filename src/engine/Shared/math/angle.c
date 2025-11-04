#include "angle.h"
#include <math.h>

void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
    float  angle;

    angle = angles[YAW] * (M_PI * 2 / 360);
    float sy = sin(angle);
    float cy = cos(angle);
    angle = angles[PITCH] * (M_PI * 2 / 360);
    float sp = sin(angle);
    float cp = cos(angle);
    angle = angles[ROLL] * (M_PI * 2 / 360);
    float sr = sin(angle);
    float cr = cos(angle);

    forward[0] = cp * cy;
    forward[1] = cp * sy;
    forward[2] = -sp;
    right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
    right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
    right[2] = -1 * sr * cp;
    up[0] = (cr * sp * cy + -sr * -sy);
    up[1] = (cr * sp * sy + -sr * cy);
    up[2] = cr * cp;
}

float anglemod(float a) {
#if 0
    if (a >= 0) a -= 360 * (int)(a / 360);
    else        a += 360 * (1 + (int)(-a / 360));
#endif
    a = (360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535);
    return a;
}
