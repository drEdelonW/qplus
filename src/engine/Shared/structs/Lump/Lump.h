#pragma once

#include "types.h"
#include "assert.h"

typedef struct {
    uint32_t fileOfs;
    uint32_t fileLen;
} Lump_t;       STATIC_ASSERT_SIZE(Lump_t, 2*4); // 8
typedef Lump_t* Lump_p;

static inline TypeLess_ptr getMapLumpPtr(TypeLess_ptr base, Lump_p lump) {
    return (TypeLess_ptr)((uint8_p)base + lump->fileOfs);
}

typedef struct {
    uint32_t ofs;   /* byte offset from start of progs blob */
    uint32_t num;   /* element count (not bytes) */
} progLump_t;       STATIC_ASSERT_SIZE(progLump_t, 2*4); // 60

void SetLumpBase(TypeLess_ptr base);
TypeLess_ptr GetPtrFromLump(progLump_t pl);