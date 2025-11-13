#include "menu_prv.h"
#include "vid.h"
#include "render.h"
#include <string.h>
#include "q_tools.h"
#include "host.h"
#include "server.h"

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/

static uint8_t _identityTable[256];
static uint8_t _translationTable[256];

void M_BuildTranslationTable(int top, int bottom) {
    for (int i = 0; i < 256; i++)
        _identityTable[i] = i;

    uint8_p dest = _translationTable;
    uint8_p source = _identityTable;
    memcpy(dest, source, 256);

    if (top < 128) // the artists made some backwards ranges.  sigh.
        memcpy(dest + TOP_RANGE, source + top, 16);
    else
        for (int i = 0; i < 16; i++)
            dest[TOP_RANGE + i] = source[top + 15 - i];

    if (bottom < 128)
        memcpy(dest + BOTTOM_RANGE, source + bottom, 16);
    else
        for (int i = 0; i < 16; i++)
            dest[BOTTOM_RANGE + i] = source[bottom + 15 - i];
}


void M_DrawTransPicTranslate(int x, int y, qPic_p pic) { Draw_TransPicTranslate(x + ((vid.width - 320) >> 1), y, pic, _translationTable); }


void M_DrawTextBox(int x, int y, int width, int lines) {
    // draw left side
    int cx = x;
    int cy = y;
    M_DrawTransPic(cx, cy, Draw_CachePic("gfx/box_tl.lmp"));
    for (int n = 0; n < lines; n++) {
        cy += D_CHAR_HEIGHT;
        M_DrawTransPic(cx, cy, Draw_CachePic("gfx/box_ml.lmp"));
    }
    M_DrawTransPic(cx, cy + D_CHAR_HEIGHT, Draw_CachePic("gfx/box_bl.lmp"));

    // draw middle
    cx += 8;
    while (width > 0) {
        cy = y;
        M_DrawTransPic(cx, cy, Draw_CachePic("gfx/box_tm.lmp"));
        qPic_p p = Draw_CachePic("gfx/box_mm.lmp");
        for (int n = 0; n < lines; n++) {
            cy += D_CHAR_HEIGHT;
            if (n == 1)
                p = Draw_CachePic("gfx/box_mm2.lmp");
            M_DrawTransPic(cx, cy, p);
        }
        M_DrawTransPic(cx, cy + 8, Draw_CachePic("gfx/box_bm.lmp"));
        width -= 2;
        cx += 16;
    }

    // draw right side
    cy = y;
    M_DrawTransPic(cx, cy, Draw_CachePic("gfx/box_tr.lmp"));
    for (int n = 0; n < lines; n++) {
        cy += D_CHAR_HEIGHT;
        M_DrawTransPic(cx, cy, Draw_CachePic("gfx/box_mr.lmp"));
    }
    M_DrawTransPic(cx, cy + 8, Draw_CachePic("gfx/box_br.lmp"));
}

void M_DrawCharacter(int cx, int line, int num) { Draw_Character(cx + ((vid.width - 320) >> 1), line, num); }

void M_DrawTransPic(int x, int y, qPic_p pic) { Draw_TransPic(x + ((vid.width - 320) >> 1), y, pic); }

int M_DrawPicHC(int y, qPic_p pic) {
    if (pic) {
        int ret = (320 - pic->width) / 2;
        M_DrawPic(ret, y, pic);
        return ret;
    }
    return -1;
}
void M_DrawPic(int x, int y, qPic_p pic) {
    Draw_Pic(x + ((vid.width - 320) >> 1), y, pic);
}

void M_Print(int cx, int cy, cStringRO str) {
    while (*str) {
        M_DrawCharacter(cx, cy, (*str) + 128);
        str++;
        cx += D_CHAR_WIDTH;
    }
}

void M_PrintWhite(int cx, int cy, cString str) {
    while (*str) {
        M_DrawCharacter(cx, cy, *str);
        str++;
        cx += D_CHAR_WIDTH;
    }
}

#define SLIDER_RANGE 10

void M_DrawSlider(int x, int y, float range) {
    CLAMP(0.0f, range, 1.0f);

    M_DrawCharacter(x - D_CHAR_WIDTH, y, 128);
    int i = 0;
    for (; i < SLIDER_RANGE; i++)
        M_DrawCharacter(x + i * D_CHAR_WIDTH, y, 129);
    M_DrawCharacter(x + i * D_CHAR_WIDTH, y, 130);

    M_DrawCharacter(x + (SLIDER_RANGE - 1) * D_CHAR_WIDTH * range, y, 131);
}


void M_DrawCheckbox(int x, int y, int on) {
    M_Print(x, y, (on) ?
#if 0
        131 : 129
        "[V]" : "[ ]"
#else
        "on" : "off"
#endif
    );
}

int curAnimFrame() { return ((int)(host_time * 10) % 6) + 1; }
int blink(char sym) { return sym + ((int)(realtime * 4) & 1); }
int curSymb() { return blink(12); }
int inpSymb() { return blink(10); }
