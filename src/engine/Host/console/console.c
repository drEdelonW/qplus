/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// console.c

#ifdef NeXT
#   include <libc.h>
#endif
#ifndef _MSC_VER
#   include <unistd.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "console.h"
#include "keys.h"
#include "menu_prv.h"
#include "client.h"
#include "q_tools.h"
#include "common.h"
#include "zone.h"
#include "cmd.h"
#include "sound.h"
#include "screen.h"
#include "host.h"
#include "sys.h"
#include "draw.h"
#include "cvar_q1.h"


#define CON_TEXTSIZE    (int32_t)0x4000 /*16Kb - 16384b*/
#define NUM_CON_TIMES 4

int32_t edit_line;

console_t con;

typedef struct {
    int32_t  linewidth;
    float    cursorspeed;
    int32_t  current;   // where next message will be printed
    uint32_t  x;         // offset in current line for next print
    cString  text;
    int32_t  vislines;
    bool     debuglog;
    float    times[NUM_CON_TIMES]; // realtime time the line was generated for transparent notify lines
} _console_t;

static _console_t _con = {
    .cursorspeed = 4
};

/*
    ================
    Con_ToggleConsole_f
    ================
*/
void Con_ToggleConsole_f() {
    if (key.dest == key_console) {
        if (cls.state == ca_connected) {
            key.dest = key_game;
            con.lines[edit_line][1] = 0; // clear any typing
            con.linepos = 1;
        }
        else    M_Menu_Main_f();
    }
    else    key.dest = key_console;

    SCR_EndLoadingPlaque();
    memset(_con.times, 0, sizeof(_con.times));
}

/*
    ================
    Con_Clear_f
    ================
*/
void Con_Clear_f() {
    if (_con.text) {
        Q_memset(_con.text, ' ', CON_TEXTSIZE);
    }
}


/*
    ================
    Con_ClearNotify
    ================
*/
void Con_ClearNotify() {
    for (int i = 0; i < NUM_CON_TIMES; i++)
        _con.times[i] = 0;
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
    Con_CheckResize

    If the line width has changed, reformat the buffer.
    ================
*/
void Con_CheckResize() {
    int32_t width = (vid.width >> 3) - 2;
    if (width == _con.linewidth)
        return;

    if (width < 1) {   // video hasn't been initialized yet
        width = 38;
        _con.linewidth = (int32_t)width;
        con.totallines = CON_TEXTSIZE / _con.linewidth;
        Q_memset(_con.text, ' ', CON_TEXTSIZE);
    }
    else {
        uint32_t oldwidth = (uint32_t)_con.linewidth;
        _con.linewidth = (int32_t)width;
        int32_t oldtotallines = con.totallines;
        con.totallines = CON_TEXTSIZE / _con.linewidth;
        int32_t numlines = oldtotallines;

        if (con.totallines < numlines)
            numlines = con.totallines;

        uint32_t numchars = oldwidth;

        if (_con.linewidth < numchars)
            numchars = (uint32_t)_con.linewidth;

        char tbuf[CON_TEXTSIZE];
        Q_memcpy(tbuf, _con.text, CON_TEXTSIZE);
        Q_memset(_con.text, ' ', CON_TEXTSIZE);

        for (int32_t i = 0; i < numlines; i++) {
            for (int32_t j = 0; j < numchars; j++) {
                _con.text[(con.totallines - 1 - i) * _con.linewidth + j] =
                    tbuf[((_con.current - i + oldtotallines) %
                        oldtotallines) * (int32_t)oldwidth + j];
            }
        }

        Con_ClearNotify();
    }

    con.backscroll = 0;
    _con.current = (int32_t)con.totallines - 1;
}


/*
    ================
    Con_Init
    ================
*/
void Con_Init() {
    _con.debuglog = COM_CheckParm("-condebug");

    if (_con.debuglog) {
        cString t2 = "/qconsole.log";
        if (strlen(com.gamedir) < (MAXGAMEDIRLEN - strlen(t2))) {
            char temp[MAXGAMEDIRLEN + 1];
            snprintf(temp, sizeof(temp), "%s%s", com.gamedir, t2);
            unlink(temp);
        }
    }

    _con.text = Hunk_AllocName(CON_TEXTSIZE, "context");
    Q_memset(_con.text, ' ', CON_TEXTSIZE);
    _con.linewidth = -1;
    Con_CheckResize();

    Con_Printf("Console initialized.\n");

    //
    // register our commands
    //
    Cvar_RegisterVariable(&con_notifytime);

    Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
    Cmd_AddCommand("messagemode", Con_MessageMode_f);
    Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
    Cmd_AddCommand("clear", Con_Clear_f);
    con.isInitialized = true;
}


/*
    ===============
    Con_Linefeed
    ===============
*/
void Con_Linefeed() {
    _con.x = 0;
    _con.current++;
    Q_memset(
        &_con.text[
            (uint32_t)((_con.current % (int32_t)con.totallines) * _con.linewidth)
        ],
        ' ', (uint32_t)_con.linewidth
    );
}

/*
    ================
    Con_Print

    Handles cursor positioning, line wrapping, etc
    All console printing must go through this in order to be logged to disk
    If no console is visible, the notify window will pop up.
    ================
*/
void Con_Print(cStringRO txt) {
    static bool cr;

    con.backscroll = 0;

    uint8_t mask;
    if (txt[0] == 1) {
        mask = 128;  // go to colored text
        S_LocalSound("misc/talk.wav");
        // play talk wav
        txt++;
    }
    else if (txt[0] == 2) {
        mask = 128;  // go to colored text
        txt++;
    }
    else    mask = 0;


    char    c;
    while ((c = *txt)) {
        // count word length
        uint32_t l = 0;
        for (; l < _con.linewidth; l++) {
            if (txt[l] <= ' ')
                break;
        }

        // word wrap
        if ((l != _con.linewidth) &&
            ((_con.x + l) > (uint32_t)_con.linewidth)) {
            _con.x = 0;
        }

        txt++;

        if (cr) {
            _con.current--;
            cr = false;
        }


        if (!_con.x) {
            Con_Linefeed();
            // mark time for transparent overlay
            if (_con.current >= 0)
                _con.times[_con.current % NUM_CON_TIMES] = (float)realtime;
        }

        switch (c) {
        case '\n':
            _con.x = 0;
            break;

        case '\r':
            _con.x = 0;
            cr = 1;
            break;

        default: // display character and advance
            _con.text[
                (uint32_t)(
                    (_con.current % (int32_t)con.totallines) *
                    _con.linewidth) +
                    _con.x
            ] = (uint32_t)c | mask;
            _con.x++;
            if (_con.x >= _con.linewidth) {
                _con.x = 0;
            }
            break;
        }

    }
}


/*
    ================
    Con_DebugLog
    ================
*/
void Con_DebugLog(cString file, cString fmt, ...) {
    static char data[1024];

    va_list argptr; va_start(argptr, fmt);
    vsnprintf(data, sizeof(data), fmt, argptr);
    va_end(argptr);

    int fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    write(fd, data, strlen(data));
    close(fd);
}


/*
    ================
    Con_Printf

    Handles cursor positioning, line wrapping, etc
    ================
*/
#define MAXPRINTMSG (4096)
void Con_Printf(cStringRO fmt, ...) {
    static bool inupdate;

    va_list argptr;         va_start(argptr, fmt);
    char msg[MAXPRINTMSG];  vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    // also echo to debugging console
    Host_Printf("%s", msg); // also echo to debugging console

    // log all messages to file
    if (_con.debuglog)
        Con_DebugLog(va("%s/qconsole.log", com.gamedir), "%s", msg);


    if ((!con.isInitialized) ||
        (cls.state == ca_dedicated)
        )
        return;  // no graphics mode


    // write it to the scrollable buffer
    Con_Print(msg);

    // update the screen if the console is displayed
    if ((cls.signon != SIGNONS) &&
        (!scr.disabled_for_loading)
        ) {
        // protect against infinite loop if something in SCR_UpdateScreen calls
        // Con_Printd
        if (!inupdate) {
            inupdate = true;
            SCR_UpdateScreen();
            inupdate = false;
        }
    }
}

/*
    ================
    Con_DPrintf

    A Con_Printf that only shows up if the "developer" cvar is set
    ================
*/
void Con_DPrintf(cStringRO fmt, ...) {
    if (!developer.value)
        return;   // don't confuse non-developers with techie stuff...

    va_list argptr;         va_start(argptr, fmt);
    char msg[MAXPRINTMSG];  vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    Con_Printf("%s", msg);
}


/*
    ==================
    Con_SafePrintf

    Okay to call even when the screen can't be updated
    ==================
*/
void Con_SafePrintf(cStringRO fmt, ...) {
    va_list argptr; va_start(argptr, fmt);
    char msg[1024]; vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    int temp = scr.disabled_for_loading;
    scr.disabled_for_loading = true;
    Con_Printf("%s", msg);
    scr.disabled_for_loading = temp;
}


/*
    ==============================================================================

    DRAWING

    ==============================================================================
*/


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

    cString text = con.lines[edit_line];

    // add the cursor frame
    text[con.linepos] = 10 + ((int)(realtime * _con.cursorspeed) & 1);

    // fill out remainder with spaces
    for (uint32_t i = (con.linepos + 1); i < _con.linewidth; i++)
        text[i] = ' ';

    // prestep if horizontally scrolling
    if (con.linepos >= _con.linewidth)
        text += 1 + con.linepos - (uint32_t)_con.linewidth;

    // draw it
    int32_t y = _con.vislines - 16;

    for (int32_t i = 0; i < _con.linewidth; i++)
        Draw_Character((i + 1) << 3, y, text[i]);

    // remove cursor
    con.lines[edit_line][con.linepos] = 0;
}


/*
    ================
    Con_DrawNotify

    Draws the last few lines of output transparently over the game top
    ================
*/
void Con_DrawNotify() {

    int32_t v = 0;
    for (int32_t i = (_con.current - NUM_CON_TIMES + 1); i <= _con.current; i++) {
        if (i < 0)  continue;

        float time = _con.times[i % NUM_CON_TIMES];
        if (time == 0)  continue;

        time = (float)realtime - time;
        if (time > con_notifytime.value)    continue;

        cString text = _con.text + (i % (int32_t)con.totallines) * _con.linewidth;

        clearnotify = 0;
        scr.copytop = 1;

        for (int32_t x = 0; x < _con.linewidth; x++)
            Draw_Character((x + 1) << 3, v, text[x]);

        v += D_CHAR_HEIGHT;
    }

    if (key.dest == key_message) {
        clearnotify = 0;
        scr.copytop = 1;

        int32_t x = 0;

        Draw_String(8, v, "say:");
        while (chatBuffer[x]) {
            Draw_Character((x + 5) << 3, v, chatBuffer[x]);
            x++;
        }
        Draw_Character((x + 5) << 3, v, 10 + ((int)(realtime * _con.cursorspeed) & 1));
        v += D_CHAR_HEIGHT;
    }

    if (v > con.notifylines) {
        con.notifylines = v;
    }
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
    _con.vislines = lines;

    int32_t rows = (lines - 16) >> 3;  // rows of text to draw
    int32_t y = lines - 16 - (rows << 3); // may start slightly negative

    for (int32_t i = (_con.current - rows + 1); i <= _con.current; i++, y += D_CHAR_HEIGHT) {
        int32_t j = i - con.backscroll;

        CLAMP_LESS(j, 0);

        cString text = _con.text + (j % con.totallines) * _con.linewidth;

        for (int32_t x = 0; x < _con.linewidth; x++)
            Draw_Character((x + 1) << 3, y, text[x]);

    }

    // draw the input prompt, user text, and cursor if desired
    if (drawinput)
        Con_DrawInput();

}


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
        double t1 = Host_FloatTime();
        SCR_UpdateScreen();
        Sys_SendKeyEvents();
        double t2 = Host_FloatTime();
        realtime += t2 - t1;    // make the cursor blink
    } while (key.count < 0);

    Con_Printf("\n");
    key.dest = key_game;
    realtime = 0;       // put the cursor back to invisible
}

