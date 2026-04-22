#pragma once
#include "types.h"
#include "assert.h"

typedef struct {
    uint32_t ofs;   /* byte offset from start of progs blob */
    uint32_t num;   /* element count (not bytes) */
} progLump_t;
STATIC_ASSERT_SIZE(progLump_t, 2*4); // 60