#pragma once
#include "types.h"

typedef struct vRect_s vRect_t;
typedef vRect_t* vRect_p;
struct vRect_s {
    int x, y;
    int width, height;
    union {
        uint8_p pBuff;
        vRect_p pnext;
    };
};