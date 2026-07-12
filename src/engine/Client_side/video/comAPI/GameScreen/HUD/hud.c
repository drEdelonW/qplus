#include "cvar_q1.h"
#include "client.h"
#include "qPic.h"
#include "draw.h"
#include "host.h"
#include "console.h"
#include "screen.h"
#include "screen_prv.h"
#include "qSymbolChar.h"



void SCR_DrawPause() {
    if ((!scr_showpause.value) ||  // turn off for screenshots
        (!cl.paused)
        )   return;
    Draw_PicCenter("gfx/pause.lmp");
}


void SCR_DrawLoading() {
    if (!_scr.drawloading)   return;
    Draw_PicCenter("gfx/loading.lmp");
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
#if 0
            Draw_Character(MUL8(x + 1), v, text[x]);
        v += D_CHAR_HEIGHT;
#else
            Draw_CharGrid((x + 1), v, text[x]);
        v++;
#endif
    }

    if (key.dest == key_message) {
        Scr.clearnotify = 0;
        Scr.copytop = true;
        int offs = 1;
#if 0
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
#else
        Draw_StrGrid(offs++, v, "say:"); {
            offs += 3; // strlen("say:") - 1?;
            CharCol_t x = 0;
            while (chatBuffer[x]) {
                Draw_CharGrid(
                    offs++, v,
                    chatBuffer[x]
                );
                x++;
            }
        }
        Draw_CharGrid(offs++, v, // Draw Input Cursor
            InputCursor_Symb + ((int)(GetRealTime() * con.cursorBlinkHz) & 1)
        );
        v++;
#endif
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
    D_BeginDirectRect(
        Scr.canvas.width - 24, 0,
        draw_disc->data,
        24, 24
    );
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
    D_EndDirectRect(
        Scr.canvas.width - 24, 0,
        24, 24
    );
#endif
}


void SCR_DrawRam() {
    if ((!scr_showram.value) ||
        (!r_cache_thrash)
        )   return;
    // printf("drawRAM [%s]  \n", r_cache_thrash ? "true" : "false");
    Draw_Pic(
        Scr.canvas.x + 32,
        Scr.canvas.y,
        _scr.ram
    );
}


void SCR_DrawTurtle() {
    if (!scr_showturtle.value)  return;

    static int _cnt;
    if (host_frametime < 0.1) {
        _cnt = 0;
        return;
    }

    _cnt++;
    if (_cnt < 3)  return;

    Draw_Pic(
        Scr.canvas.x,
        Scr.canvas.y,
        _scr.turtle
    );
}


void SCR_DrawNet() {
    if (((GetRealTime() - cl.last_received_message) < 0.3f) ||
        (cls.isDemoPlaying))
        return;

    Draw_Pic(
        Scr.canvas.x + 64,
        Scr.canvas.y,
        _scr.net
    );
}

