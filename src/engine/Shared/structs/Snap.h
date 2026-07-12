#pragma once

typedef enum {
    invSpan     = -1,   // = in inverted span (end before start)
    notInSpan   = 0,    // = not in span
    inSpan      = 1,    // = in span
} SnapState_e;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct sSpan_s {
    int u;
    int v;
    int count;
} sSpan_t;
typedef sSpan_t* sSpan_p;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct eSpan_s eSpan_t;
typedef eSpan_t* eSpan_p;
struct eSpan_s {
    eSpan_p pnext;
    int     u;
    int     v;
    int     count;
};



void D_DrawSpans8(eSpan_p  pspans);
void D_DrawSpans16(eSpan_p pspans);
void D_DrawZSpans(eSpan_p  pspans);
void Turbulent8(eSpan_p    pspan);
void D_SpriteDrawSpans(sSpan_p pspan);

void D_DrawSkyScans8(eSpan_p   pspan);
void D_DrawSkyScans16(eSpan_p  pspan);

extern void(*d_drawspans)(eSpan_p pspan);