#include "draw.h"
#include "qSymbolChar.h"

qColor8_p pDrawChars;
qPic_p draw_disc;

void Draw_CharGrid(int col, int row, ConsoleSymbols_t symb) {
    Draw_Character(MUL8(col), MUL8(row), symb);
}

void Draw_CharToConback(ConsoleSymbols_t symb, qColor8_p dest) {
    qColor8_p source = CharGlyphSource(symb);
    int drawline = 8;
    while (drawline--) {
        for (int x = 0; x < 8; x++)
#ifdef GLQUAKE
            if (source[x].i != InkTransp)       // OpenGL Render
#else
            if (source[x].i != InkConTransp)    // Soft Render
#endif
                dest[x].i = 0x60 + source[x].i;
        source += 128;
        dest += 320;
    }
}

/*
================
Draw_String
================
*/
void Draw_String(int x, int y, cStringRO str) {
    while (*str) {
        Draw_Character(x, y, *str);
        str++;
        x += 8;
    }
}