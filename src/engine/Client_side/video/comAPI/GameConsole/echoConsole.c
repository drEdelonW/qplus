#include "screen.h"
#include "screen_prv.h"
#include "keys.h"
#include "draw.h"
#include "console.h"
#include "q_tools.h"
#include "host.h"
#include "cvar_q1.h"
#include "client.h"
#include "menu_prv.h"
#include <string.h>
#include "sys.h"


/*
    ==================
    Con_NotifyBox
    ==================
*/
void Con_NotifyBox(cString text) {
    // during startup for sound / cd warnings
    Con_Printf("\n\n" CON_HORIZONLINE);

    Con_Printf(text);

    Con_Printf("Press a key.\n");
    Con_Printf(CON_HORIZONLINE);

    key.count = -2; // wait for a key down and up
    key.dest = key_console;

    do {
        LegacyTimeStamp_t t1 = Host_FloatTime();
        SCR_UpdateScreen();
        Sys_SendKeyEvents();
        LegacyTimeStamp_t t2 = Host_FloatTime();
        realtime += t2 - t1;    // make the cursor blink
    } while (key.count < 0);

    Con_Printf("\n");
    key.dest = key_game;
    realtime = 0;       // put the cursor back to invisible
}

/*
    ================
    Con_CheckResize

    If the line width has changed, reformat the buffer.
    ================
*/
void Con_CheckResize() {
    int32_t width = (vid.scr.width >> 3) - 2;
    if (width == con.linewidth)
        return;

    if (width < 1) {   // video hasn't been initialized yet
        width = 38;
        con.linewidth = (int32_t)width;
        con.totallines = CON_TEXTSIZE / con.linewidth;
        Q_memset(con.text, ' ', CON_TEXTSIZE);
    }
    else {
        uint32_t oldwidth = (uint32_t)con.linewidth;
        con.linewidth = (int32_t)width;
        int32_t oldtotallines = con.totallines;
        con.totallines = CON_TEXTSIZE / con.linewidth;
        int32_t numlines = oldtotallines;

        if (con.totallines < numlines)
            numlines = con.totallines;

        uint32_t numchars = oldwidth;

        if (con.linewidth < numchars)
            numchars = (uint32_t)con.linewidth;

        char tbuf[CON_TEXTSIZE];
        Q_memcpy(tbuf, con.text, CON_TEXTSIZE);
        Q_memset(con.text, ' ', CON_TEXTSIZE);

        for (int32_t i = 0; i < numlines; i++) {
            for (int32_t j = 0; j < numchars; j++) {
                con.text[(con.totallines - 1 - i) * con.linewidth + j] =
                    tbuf[((con.current - i + oldtotallines) %
                        oldtotallines) * (int32_t)oldwidth + j];
            }
        }

        Con_ClearNotify();
    }

    con.backscroll = 0;
    con.current = (int32_t)con.totallines - 1;
}

/*
    ================
    Con_MessageMode_f
    ================
*/

void Con_MessageMode_f() {
    key.dest = key_message;
    team_message = false;
}


/*
    ================
    Con_MessageMode2_f
    ================
*/
void Con_MessageMode2_f() {
    key.dest = key_message;
    team_message = true;
}

/*
    ================
    Con_ToggleConsole_f
    ================
*/
void Con_ToggleConsole_f() {
    if (key.dest == key_console) {
        if (cls.state == ca_connected) {
            key.dest = key_game;
            con.lines[con.edit_line][1] = 0; // clear any typing
            con.linepos = 1;
        }
        else    M_Menu_Main_f();
    }
    else    key.dest = key_console;

    SCR_EndLoadingPlaque();
    memset(con.times, 0, sizeof(con.times));
}
/*
    ================
    Con_DrawInput

    The input line scrolls horizontally if typing goes beyond the right edge
    ================
*/
void Con_DrawInput() {
    if ((key.dest != key_console) &&
        (!con.forcedup)) {
        return;  // don't draw anything
    }

    cString text = con.lines[con.edit_line];

    // add the cursor frame
    text[con.linepos] = 10 + ((int)(realtime * con.cursorspeed) & 1);

    // fill out remainder with spaces
    for (uint32_t i = (con.linepos + 1); i < con.linewidth; i++)
        text[i] = ' ';

    // prestep if horizontally scrolling
    if (con.linepos >= con.linewidth)
        text += 1 + con.linepos - (uint32_t)con.linewidth;

    // draw it
    int32_t y = con.vislines - 16;

    for (int32_t i = 0; i < con.linewidth; i++)
        Draw_Character((i + 1) << 3, y, text[i]);

    // remove cursor
    con.lines[con.edit_line][con.linepos] = 0;
}

/*
    ================
    Con_DrawConsole

    Draws the console with the solid background
    The typing input line at the bottom should only be drawn if typing is allowed
    ================
*/
void Con_DrawConsole(int32_t lines, bool drawinput) {
    if (lines <= 0)
        return;

    // draw the background
    Draw_ConsoleBackground(lines);

    // draw the text
    con.vislines = lines;

    int32_t rows = (lines - 16) >> 3;  // rows of text to draw
    int32_t y = lines - 16 - (rows << 3); // may start slightly negative

    for (int32_t i = (con.current - rows + 1); i <= con.current; i++, y += D_CHAR_HEIGHT) {
        int32_t j = i - con.backscroll;

        CLAMP_LESS(j, 0);

        cString text = con.text + (j % con.totallines) * con.linewidth;

        for (int32_t x = 0; x < con.linewidth; x++)
            Draw_Character((x + 1) << 3, y, text[x]);

    }

    // draw the input prompt, user text, and cursor if desired
    if (drawinput)
        Con_DrawInput();

}





/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole() {
    if (scr.con_current) {
        scr.copyeverything = true;
        Con_DrawConsole(scr.con_current, true);
        _scr.clearConsole = 0;
    }
    else {
        if ((key.dest == key_game) ||
            (key.dest == key_message)
            )
            Con_DrawNotify();   // only draw notify in game
    }
}
