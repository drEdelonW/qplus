#include "cvar_q1.h"
#include "client.h"
#include "qPic.h"
#include "draw.h"
#include "screen_prv.h"


/*
==============
DrawPause
==============
*/
void SCR_DrawPause() {
    if ((!scr_showpause.value) ||  // turn off for screenshots
        (!cl.paused))
        return;

    qPic_p pic = Draw_CachePic("gfx/pause.lmp");
    Draw_Pic((vid.width - pic->width) / 2,
        (vid.height - 48 - pic->height) / 2, pic);
}



/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading() {
    if (!_scr.drawloading)   return;

    qPic_p pic = Draw_CachePic("gfx/loading.lmp");
    Draw_Pic((vid.width - pic->width) / 2,
        (vid.height - 48 - pic->height) / 2, pic);
}
