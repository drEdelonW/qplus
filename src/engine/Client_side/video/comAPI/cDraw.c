#include "draw.h"
#include "qSymbolChar.h"
#include "screen.h"

qColor8_p pDrawChars;
qPic_p draw_disc;

void Draw_CharGrid(int col, int row, ConsoleSymbols_t symb) {
    Draw_Character(MUL8(col), MUL8(row), symb);
}

void Draw_PicCenter(cStringRO str) {
    qPic_p pic = Draw_CachePic(str);
    Draw_Pic(
        HALF(Scr.canvas.width - pic->width),
        HALF(Scr.canvas.height - pic->height) - 24,
        pic
    );
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


void Draw_String(int x, int y, cStringRO str) {
    while (*str) {
        Draw_Character(x, y, *str);
        str++;
        x += 8;
    }
}


void Draw_StrGrid(int col, int row, cStringRO str) {
    Draw_String(MUL8(col), MUL8(row), str);
}


void Draw_PicName(int x, int y, cStringRO str) {
    qPic_p pic = Draw_CachePic(str);
    Draw_Pic(x, y, pic);
}

void Draw_TransPicName(int x, int y, cStringRO str) {
    qPic_p pic = Draw_CachePic(str);
    Draw_TransPic(x, y, pic);
}