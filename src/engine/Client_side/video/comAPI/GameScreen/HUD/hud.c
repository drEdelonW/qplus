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
void SCR_DrawPause() {
    if ((!scr_showpause.value) ||  // turn off for screenshots
        (!cl.paused)
        )   return;

    qPic_p pic = Draw_CachePic("gfx/pause.lmp");
    Draw_Pic(   // TODO: wrap it to Draw_PicCenter(pic-cString)
        HALF(Scr.vrect.width - pic->width),
        HALF(Scr.vrect.height - pic->height - 48),
        pic
    );
}



/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading() {
    if (!_scr.drawloading)   return;

    qPic_p pic = Draw_CachePic("gfx/loading.lmp");
    Draw_Pic(   // TODO: wrap it to Draw_PicCenter(pic-cString)
        HALF(Scr.vrect.width - pic->width),
        HALF(Scr.vrect.height - pic->height - 48),
        pic
    );
}


/*
    ================
    Con_DrawNotify

    Draws the last few lines of output transparently over the game top
    ================
*/
void Con_DrawNotify() {
    CmdLine_t v = FirstLine;
    for (int i = (con.current - NUM_CON_TIMES + 1); i <= con.current; i++) {
        if (i < 0)  continue;

        sRealTime_t time = con.times[i % NUM_CON_TIMES];
        if (time == 0)  continue;

        time = GetRealTime() - time;
        if (time > con_notifytime.value)    continue;

        cString text = con.pText + (i % con.totallines) * con.linewidth;

        Scr.clearnotify = 0;
        Scr.copytop = true;

        for (CharCol_t x = FirstChar; x < con.linewidth; x++)
            Draw_Character(MUL8(x + 1), v, text[x]);

        v += D_CHAR_HEIGHT;
    }

    if (key.dest == key_message) {
        Scr.clearnotify = 0;
        Scr.copytop = true;

        Draw_String(8, v, "say:"); {
            CharCol_t x = FirstChar;
            while (chatBuffer[x]) {
                Draw_Character(MUL8(x + 5), v,
                    chatBuffer[x]);
                x++;
            }
            Draw_Character(MUL8(x + 5), v, // Draw Input Cursor
                InputCursor_Symb + ((int)(GetRealTime() * con.cursorBlinkHz) & 1)
            );
        }
        v += D_CHAR_HEIGHT;
    }

    ClampLessThen(&con.notifylines, v);
}