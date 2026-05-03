#pragma once


// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct eSpan_s eSpan_t;
typedef eSpan_t* eSpan_p;
struct eSpan_s {
    int     u;
    int     v;
    int     count;
    eSpan_p pnext;
};
