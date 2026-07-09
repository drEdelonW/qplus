// #include "render.h"
#include "screen.h"
#include "client.h"
#include "cvar_q1.h"
#include "GameRule.h"


/*
===============
R_SetVrect
===============
*/
void R_SetVrect(const vRect_p pvrectin, vRect_p pRect, int lineadj) {
#ifdef GLQUAKE
    bool full = ((scr_viewsize.value >= 100.0f) || (isIntermission()));
#endif
    float size = (scr_viewsize.value > 100.0f) ?
        100.0f : scr_viewsize.value;

    if (isIntermission()) {
        size = 100.0f;
        lineadj = 0;
    }
    size /= 100.0f; // normalize to 1.f

    int h = pvrectin->height - lineadj;
    pRect->width = pvrectin->width * size;
    if (pRect->width < 96.0f) {
        size = 96.0f / pvrectin->width;
        pRect->width = 96.0f; // min for icons
    }
    pRect->width &= ~7;
    pRect->height = pvrectin->height * size;
    CLAMP_MORE(&pRect->height, (pvrectin->height - lineadj));

#ifdef GLQUAKE
    {   /* GLQUAKE specific */
        if (pRect->height > pvrectin->height)
            pRect->height = pvrectin->height;

        pRect->x = HALF(pvrectin->width - pRect->width);
        pRect->y = (full) ? 0 : HALF(h - pRect->height);
    }
#else
    {   /* Soft Render specific */
        pRect->height &= ~1;

        pRect->x = HALF(pvrectin->width - pRect->width);
        pRect->y = HALF(h - pRect->height);

# ifndef STM32
        if (lcd_x.value) {
            pRect->y = HALF(pRect->y);
            pRect->height = HALF(pRect->height);
        }
# endif
    }
#endif
}