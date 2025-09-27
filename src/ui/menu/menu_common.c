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

void M_DrawTransPic(int x, int y, qpic_p pic) {
    Draw_TransPic(x + ((vid.width - 320) >> 1), y, pic);
}

void M_DrawPic(int x, int y, qpic_p pic) {
    Draw_Pic(x + ((vid.width - 320) >> 1), y, pic);
}