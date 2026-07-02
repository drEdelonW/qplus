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

#include "mathlib.h"

static inline BBox_t BBoxOrig() { return (BBox_t){ .mins = vec3_origin, .maxs = vec3_origin }; }
static inline BBox_t BBoxTranslate(BBox_t bb, vec3_t offset) {
    return (BBox_t) {
        .mins = VectorAdd(bb.mins, offset),
        .maxs = VectorAdd(bb.maxs, offset)
    };
}