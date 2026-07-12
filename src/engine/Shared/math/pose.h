#pragma once
#include "vector.h"
#include "angle.h"

typedef struct {
    vec3_t  loc;    // "position" ex [Origin/Org]
    ang3_t  aim;    // "orientation" ex [Angles]
} pose_t;
typedef pose_t* pose_p;

// extern const pose_t poseZero;

static inline pose_t PoseMA(pose_t base, float scale, pose_t delta) {
    return (pose_t){
        .loc   = VectorMA(base.loc, scale, delta.loc),
        .aim = AngleMA(base.aim, scale, delta.aim),
    };
}

static inline bool PoseCompare(pose_t const a, pose_t const b) {
    return VectorCompare(a.loc, b.loc) && AngleCompare(a.aim, b.aim);
}

static inline pose_t PoseLerp(pose_t a, pose_t b, float t) {
    return PoseMA(a, t, (pose_t){
        .loc   = VectorSubtract(b.loc, a.loc),
        .aim = AngleSubtract(b.aim, a.aim),
    });
}