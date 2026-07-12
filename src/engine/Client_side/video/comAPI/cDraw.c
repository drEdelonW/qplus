#include "draw.h"
#include "qSymbolChar.h"
#include "screen.h"

qColor8_p pDrawChars;

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

void Draw_PicName(int x, int y, cStringRO str) {
    Draw_Pic(x, y, Draw_CachePic(str));
}

void Draw_TransPicName(int x, int y, cStringRO str) {
    Draw_TransPic(x, y, Draw_CachePic(str));
}