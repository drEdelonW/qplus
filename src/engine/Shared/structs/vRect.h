#pragma once
#include "types.h"
#include "qColor.h"

typedef struct vRect_s vRect_t;
typedef vRect_t* vRect_p;
struct vRect_s {
    int x, y;
    int width, height;
    ptrdiff_t rowBytes;  // Width in bytes - new line offset
    union {
        qColor8_p pClr;
        uint8_p pBuff;
#if 1   /* seems like  it used only in vid_x.c */
        vRect_p pNext;
#endif
    };
};