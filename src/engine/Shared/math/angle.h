#pragma once
#include <math.h>   // sqrtf, fabsf

#ifndef M_PI
# define M_PI  (3.14159265358979323846) /* matches value in gcc v2 math.h */
#endif
#define DEG2RAD(a) (a * M_PI) / 180.0F
#define RAD2DEG(a) (a / M_PI) * 180.0F

#if 0 /* TODO: wrap angles type to avoid tipless float geting/setting */
typedef struct { float _rad; } Angle_t;

static inline float   GetRad(Angle_t a) { return a._rad; }
static inline Angle_t SetRad(float r) { return (Angle_t) { ._rad = r }; }

static inline float   GetDeg(Angle_t a) { return RAD2DEG(a._rad); }
static inline Angle_t SetDeg(float d) { return (Angle_t) { ._rad = DEG2RAD(d) }; }
#else
typedef float  Angle_t;
#endif

/* Euler angle indices: up/down, left/right, roll (fall over) */
enum {
    PITCH   = 0u,  /* up/down */
    YAW     = 1u,  /* left/right */
    ROLL    = 2u,  /* roll (fall over) */
    ANGLES_COUNT = 3u
};
typedef union {
    struct { Angle_t pitch, yaw, roll; };
    Angle_t v[ANGLES_COUNT];
} ang3_t;
typedef ang3_t* ang3_p;

extern const ang3_t a3Zero; // zero angles;

#ifdef __cplusplus
extern "C" {
#endif

    Angle_t anglemod(Angle_t a);
    Angle_t angledelta(Angle_t a);
    ang3_t  AngleProc(ang3_t angles);

#ifdef __cplusplus
}
#endif

static inline ang3_t AngleAdd(ang3_t a, ang3_t b) {
    return (ang3_t){
        .pitch = angledelta(a.pitch + b.pitch),
        .yaw   = angledelta(a.yaw   + b.yaw),
        .roll  = angledelta(a.roll  + b.roll),
    };
}

static inline ang3_t AngleSubtract(ang3_t a, ang3_t b) {
    return (ang3_t){
        .pitch = angledelta(a.pitch - b.pitch),
        .yaw   = angledelta(a.yaw   - b.yaw),
        .roll  = angledelta(a.roll  - b.roll),
    };
}

static inline ang3_t AngleMA(ang3_t a, float scale, ang3_t b) {
    return (ang3_t){
        .pitch = angledelta(a.pitch + scale * b.pitch),
        .yaw   = angledelta(a.yaw   + scale * b.yaw),
        .roll  = angledelta(a.roll  + scale * b.roll),
    };
}

static inline ang3_t AngleScale(ang3_t a, float s) {
    return (ang3_t){
        .pitch = angledelta(a.pitch * s),
        .yaw   = angledelta(a.yaw   * s),
        .roll  = angledelta(a.roll  * s)
    };
}
