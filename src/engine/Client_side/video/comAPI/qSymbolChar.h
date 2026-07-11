#pragma once

#define D_CHAR_WIDTH  (8)
#define D_CHAR_HEIGHT (8)

#define CON_HORIZONLINE "\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n"

typedef enum {
    InputCursor_Symb  = 10,
    MenuCursor_Symb   = 12,
    ConLineLeft_Symb  = 29,   // was \35
    ConLineFill_Symb  = 30,   // was \36
    ConLineRight_Symb = 31,   // was \37
} ConsoleSymbols_t;

#include "qColor.h"
extern qColor8_p pDrawChars;    // 8*8 graphic characters

static inline qColor8_p CharGlyphSource(ConsoleSymbols_t symb) {
    int num = (int)symb;
    num &= 0xFF;
    int row = (num & 0x0F) >> 4;
    int col = (num & 0x0F) >> 0;
    return pDrawChars + (row << 10) + (col << 3);
}