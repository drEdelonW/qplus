#pragma once
#include "vector.h"
#include "angle.h"

typedef struct {
    vec3_t  spot;      // "position" ex [Origin/Org]
    ang3_t  facing;    // "orientation" ex [Angles]
} pose_t;
typedef pose_t* pose_p;

// extern const pose_t poseZero;

static inline pose_t PoseMA(pose_t base, float scale, pose_t delta) {
    return (pose_t){
        .spot   = VectorMA(base.spot, scale, delta.spot),
        .facing = AngleMA(base.facing, scale, delta.facing),
    };
}

static inline bool PoseCompare(pose_t const a, pose_t const b) {
    return VectorCompare(a.spot, b.spot) && AngleCompare(a.facing, b.facing);
}

static inline pose_t PoseLerp(pose_t a, pose_t b, float t) {
    return PoseMA(a, t, (pose_t){
        .spot   = VectorSubtract(b.spot, a.spot),
        .facing = AngleSubtract(b.facing, a.facing),
    });
}