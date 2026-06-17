#include "angle.h"
#include <math.h>

void AngleVectors(vec3_t angles, vec3_p forward, vec3_p right, vec3_p up) {
#if 0
    angles.yaw *= (M_PI * 2 / 360);
    angles.pitch *= (M_PI * 2 / 360);
    angles.roll *= (M_PI * 2 / 360);
#else
    angles = VectorScale(angles, (M_PI * 2 / 360));
#endif
    float sy = sin(angles.yaw);
    float cy = cos(angles.yaw);
    float sp = sin(angles.pitch);
    float cp = cos(angles.pitch);
    float sr = sin(angles.roll);
    float cr = cos(angles.roll);

#if 0
    forward->x = cp * cy;
    forward->y = cp * sy;
    forward->z = -sp;
    right->x = (-sr * sp * cy) + (cr * sy);
    right->y = (-sr * sp * sy) + (-cr * cy);
    right->z = (-sr * cp);
    up->x = (cr * sp * cy) + (sr * sy);
    up->y = (cr * sp * sy) + (-sr * cy);
    up->z = cr * cp;
#else
    *forward = (vec3_t){
        .x = cp * cy,
        .y = cp * sy,
        .z = -sp
    };
    *right = (vec3_t){
        .x = (-sr * sp * cy) + (cr * sy),
        .y = (-sr * sp * sy) + (-cr * cy),
        .z = (-sr * cp)
    };
    *up = (vec3_t){
        .x = (cr * sp * cy) + (sr * sy),
        .y = (cr * sp * sy) + (-sr * cy),
        .z = cr * cp
    };
#endif
}

float anglemod(float a) {
#if 0
    if (a >= 0) a -= 360 * (int)(a / 360);
    else        a += 360 * (1 + (int)(-a / 360));
#endif
    a = (360.0f / (0xFFFF + 1)) * ((int)(a * ((0xFFFF + 1) / 360.0f)) & 0xFFFF);
    return a;
}
