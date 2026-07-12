#include "cvar_q1.h"
#include "client.h"
#include "qPic.h"
#include "draw.h"
#include "host.h"
#include "console.h"
#include "screen.h"
#include "screen_prv.h"
#include "qSymbolChar.h"


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
        HALF(Scr.canvas.width - pic->width),
        HALF(Scr.canvas.height - pic->height - 48),
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
        HALF(Scr.canvas.width - pic->width),
        HALF(Scr.canvas.height - pic->height - 48),
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
        int offs = 1;
        Draw_String(MUL8(offs++), v, "say:"); {
            offs += 3; // strlen("say:") - 1?;
            CharCol_t x = 0;
            while (chatBuffer[x]) {
                Draw_Character(
                    MUL8(offs++), v,
                    chatBuffer[x]
                );
                x++;
            }
        }
        Draw_Character(MUL8(offs++), v, // Draw Input Cursor
            InputCursor_Symb + ((int)(GetRealTime() * con.cursorBlinkHz) & 1)
        );
        v += D_CHAR_HEIGHT;
    }

    ClampLessThen(&con.notifylines, v);
}

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
#ifdef GLQUAKE
#include "qOpenGL.h"
#endif
void Draw_BeginDisc() {
#ifdef GLQUAKE
    if (!draw_disc)     return;

    glDrawBuffer(GL_FRONT);
    Draw_Pic(Scr.canvas.width - 24, 0, draw_disc);
    glDrawBuffer(GL_BACK);
#else
    D_BeginDirectRect(Scr.canvas.width - 24, 0, draw_disc->data, 24, 24);
#endif
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc() {
#ifndef GLQUAKE
    D_EndDirectRect(Scr.canvas.width - 24, 0, 24, 24);
#endif
}
