#include "cvar_q1.h"
#include "client.h"
#include "qPic.h"
#include "draw.h"
#include "host.h"
#include "console.h"
#include "screen.h"
#include "screen_prv.h"


/*
==============
DrawPause
==============
*/
#include "vid.h" // vid.scr.width
void SCR_DrawPause() {
    if ((!scr_showpause.value) ||  // turn off for screenshots
        (!cl.paused))
        return;

    qPic_p pic = Draw_CachePic("gfx/pause.lmp");
    Draw_Pic(HALF(vid.scr.width - pic->width),
        HALF(vid.scr.height - 48 - pic->height), pic);
}



/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading() {
    if (!_scr.drawloading)   return;

    qPic_p pic = Draw_CachePic("gfx/loading.lmp");
    Draw_Pic(HALF(vid.scr.width - pic->width),
        HALF(vid.scr.height - 48 - pic->height), pic);
}


/*
    ================
    Con_DrawNotify

    Draws the last few lines of output transparently over the game top
    ================
*/
void Con_DrawNotify() {
    int32_t v = 0;
    for (int32_t i = (con.current - NUM_CON_TIMES + 1); i <= con.current; i++) {
        if (i < 0)  continue;

        sRealTime_t time = con.times[i % NUM_CON_TIMES];
        if (time == 0)  continue;

        time = GetRealTime() - time;
        if (time > con_notifytime.value)    continue;

        cString text = con.text + (i % (int32_t)con.totallines) * con.linewidth;

        Scr.clearnotify = 0;
        Scr.copytop = true;

        for (int32_t x = 0; x < con.linewidth; x++)
            Draw_Character(OCTO(x + 1), v, text[x]);

        v += D_CHAR_HEIGHT;
    }

    if (key.dest == key_message) {
        Scr.clearnotify = 0;
        Scr.copytop = true;

        int32_t x = 0;

        Draw_String(8, v, "say:");
        while (chatBuffer[x]) {
            Draw_Character(OCTO(x + 5), v, chatBuffer[x]);
            x++;
        }
        Draw_Character(OCTO(x + 5), v, 10 + ((int)(GetRealTime() * con.cursorspeed) & 1));
        v += D_CHAR_HEIGHT;
    }

    if (v > con.notifylines) {
        con.notifylines = v;
    }
}