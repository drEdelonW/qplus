// #include "render.h"
#include "screen.h"
#include "client.h"
#include "cvar_q1.h"


/*
===============
R_SetVrect
===============
*/
void R_SetVrect(vRect_p pvrectin, vRect_p pvrect, int lineadj) {
    float size = (scr_viewsize.value > 100.0f) ?
        100.0f : scr_viewsize.value;

    if (cl.intermission != IM_NONE) {
        size = 100.0f;
        lineadj = 0;
    }
    size /= 100.0f;

    int h = pvrectin->height - lineadj;
    pvrect->width = pvrectin->width * size;
    if (pvrect->width < 96.0f) {
        size = 96.0f / pvrectin->width;
        pvrect->width = 96.0f; // min for icons
    }
    pvrect->width &= ~7;
    pvrect->height = pvrectin->height * size;
    if (pvrect->height > (pvrectin->height - lineadj))
        pvrect->height = (pvrectin->height - lineadj);

    {   /* Soft Render specific */
        pvrect->height &= ~1;

        pvrect->x = HALF(pvrectin->width - pvrect->width);
        pvrect->y = HALF(h - pvrect->height);

#ifndef STM32
        if (lcd_x.value) {
            pvrect->y = HALF(pvrect->y);
            pvrect->height = HALF(pvrect->height);
        }
#endif
    }
}