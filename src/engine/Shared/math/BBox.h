#pragma once
#include "vector.h"

typedef union {
    struct {
        vec3_t min;
        vec3_t max;
    };
    struct {
        vec3_t mins;
        vec3_t maxs;
    };
    // TODO: make union vec3_t bounds[2]
    vec_t v[6];
} BBox_t;
typedef BBox_t* BBox_p;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

static inline BBox_t BBoxTranslate(BBox_t bb, vec3_t offset) {
    return (BBox_t) {
        .mins = VectorAdd(bb.mins, offset),
        .maxs = VectorAdd(bb.maxs, offset)
    };
}