#pragma once

#include "types.h"

typedef struct {
    int32_t fileOfs;
    int32_t fileLen;
} Lump_t;
typedef Lump_t* Lump_p;
