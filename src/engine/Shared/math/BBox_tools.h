#pragma once
#include "BBox.h"
#include "progdefs.h"

static inline BBox_t EvBBox(const entvars_t *ev){
    return (BBox_t){
        .mins = ev->mins,
        .maxs = ev->maxs
    };
}

static inline void EvSetBBox(entvars_t *ev, BBox_t bb){
    ev->mins = bb.mins;
    ev->maxs = bb.maxs;
    ev->size = BBoxSize(bb);
}

static inline BBox_t EvAbsBBox(const entvars_t *ev) {
    return (BBox_t){
        .mins = ev->absmin,
        .maxs = ev->absmax
    };
}
