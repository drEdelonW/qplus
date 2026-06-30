#pragma once

#include "assert.h"
#include "types.h"

typedef struct {
    uint32_t fileOfs;
    uint32_t fileLen;
} Lump_t;
typedef Lump_t* Lump_p;
STATIC_ASSERT_SIZE(Lump_t, 2*4); // 8

static inline TypeLess_ptr getMapLumpPtr(TypeLess_ptr base, Lump_p lump) {
    return (TypeLess_ptr)((uint8_p)base + lump->fileOfs);
}
