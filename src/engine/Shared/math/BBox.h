#pragma once
#include "vector.h"

typedef union {
    struct {
        vec3_t mins;
        vec3_t maxs;
    };
    vec3_t bounds[2];   // FYI: used for BoxOnPlaneSide()
    vec_t v[6];         // TODO: remake pfrustum_indexes[] with R_RecursiveWorldNode() care
} BBox_t;
typedef BBox_t* BBox_p;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


extern const BBox_t bbNull; // inverted BoundingBox - degenerate/non-existent
extern const BBox_t bbZero; // zero BoundingBox;

static inline bool BBoxIsValid(BBox_t bb) {
    for (int i = 0; i < VECT_DIM; i++)
        if (bb.mins.v[i] > bb.maxs.v[i]) return false;
    return true;
}

static inline BBox_t BBoxFromVec3(vec3_t mins, vec3_t maxs) {
    return (BBox_t) {
        .mins = mins,
        .maxs = maxs
    };
}

static inline BBox_t BBoxTranslate(BBox_t bb, vec3_t offset) {
    return (BBox_t) {
        .mins = VectorAdd(bb.mins, offset),
        .maxs = VectorAdd(bb.maxs, offset)
    };
}

static inline BBox_t BBoxSymmetric(vec_t r) {
    return (BBox_t) {
        .mins = Scalar2Vector(-r),
        .maxs = Scalar2Vector( r)
    };
}

static inline void BBoxExpandPt(BBox_p bb, vec3_t p) {
    for (int i = 0; i < VECT_DIM; i++) {
        if (p.v[i] < bb->mins.v[i])      bb->mins.v[i] = p.v[i];
        if (p.v[i] > bb->maxs.v[i])      bb->maxs.v[i] = p.v[i];
    }
}

static inline void BoundPoly(int numverts, vec3_p verts, BBox_p bb) {
    *bb = bbNull;
    for (int i = 0; i < numverts; i++) {
        BBoxExpandPt(bb, verts[i]);
    }
}

static inline vec3_t BBoxSize(BBox_t bb) {
    return VectorSubtract(bb.maxs, bb.mins);
}

static inline vec3_t BBoxMid(BBox_t bb) {
    return VectorScale(VectorAdd(bb.mins, bb.maxs), 0.5f);
}

