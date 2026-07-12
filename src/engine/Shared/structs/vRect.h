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
        int16_p pZBuff;
#if 1   /* seems like  it used only in vid_x.c */
        vRect_p pNext;
#endif
    };
};

#ifdef __cplusplus
extern "C" {
#endif

    void D_BeginDirectRect(int x, int y, qColor8_p pbitmap, int width, int height); // mokked in vid_null.c

#ifdef __cplusplus
}
#endif