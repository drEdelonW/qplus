#include "menu_prv.h"

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
#include "vid.h"
void M_DrawCharacter(int cx, int line, int num) {
    Draw_Character(cx + ((vid.width - 320) >> 1), line, num);
}

void M_DrawTransPic(int x, int y, qPic_p pic) {
    Draw_TransPic(x + ((vid.width - 320) >> 1), y, pic);
}

void M_DrawPic(int x, int y, qPic_p pic) {
    Draw_Pic(x + ((vid.width - 320) >> 1), y, pic);
}

void M_Print(int cx, int cy, cString str) {
    while (*str) {
        M_DrawCharacter(cx, cy, (*str) + 128);
        str++;
        cx += 8;
    }
}

void M_PrintWhite(int cx, int cy, cString str) {
    while (*str) {
        M_DrawCharacter(cx, cy, *str);
        str++;
        cx += 8;
    }
}

#define SLIDER_RANGE 10

void M_DrawSlider(int x, int y, float range) {
    CLAMP(0.0f, range, 1.0f);

    M_DrawCharacter(x - 8, y, 128);
    int i = 0;
    for (; i < SLIDER_RANGE; i++)
        M_DrawCharacter(x + i * 8, y, 129);
    M_DrawCharacter(x + i * 8, y, 130);
    M_DrawCharacter(x + (SLIDER_RANGE - 1) * 8 * range, y, 131);
}


void M_DrawCheckbox(int x, int y, int on) {
#if 0
    M_Print(x, y, (on) ? 131 : 129);
#endif
    M_Print(x, y, (on) ? "on" : "off");
}
