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
// #include "keys.h"
#include "client.h"
#include "q_tools.h"
#include "common.h"
#include "zone.h"
#include "cmd.h"
#include "sound.h"
#include "screen.h"
#include "host.h"
#include "sys.h"
// #include "draw.h"
#include "cvar_q1.h"
#include "z_hunk.h"

#define CON_TEXTSIZE    (int32_t)0x4000 /*16Kb - 16384b*/


int32_t edit_line;

console_t con = {
    .cursorspeed = 4
};

typedef struct {
    uint32_t  x;         // offset in current line for next print
    bool     debuglog;
} _console_t;

static _console_t _con;



/*
    ================
    Con_Clear_f
    ================
*/
void Con_Clear_f() {
    if (con.text) {
        Q_memset(con.text, ' ', CON_TEXTSIZE);
    }
}


/*
    ================
    Con_ClearNotify
    ================
*/
void Con_ClearNotify() {
    for (int i = 0; i < NUM_CON_TIMES; i++)
        con.times[i] = 0;
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

    con.text = Hunk_AllocName(CON_TEXTSIZE, "context");
    Q_memset(con.text, ' ', CON_TEXTSIZE);
    con.linewidth = -1;
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
    con.current++;
    Q_memset(
        &con.text[
            (uint32_t)((con.current % (int32_t)con.totallines) * con.linewidth)
        ],
        ' ', (uint32_t)con.linewidth
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
        for (; l < con.linewidth; l++) {
            if (txt[l] <= ' ')
                break;
        }

        // word wrap
        if ((l != con.linewidth) &&
            ((_con.x + l) > (uint32_t)con.linewidth)) {
            _con.x = 0;
        }

        txt++;

        if (cr) {
            con.current--;
            cr = false;
        }


        if (!_con.x) {
            Con_Linefeed();
            // mark time for transparent overlay
            if (con.current >= 0)
                con.times[con.current % NUM_CON_TIMES] = (float)realtime;
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
            con.text[
                (uint32_t)(
                    (con.current % (int32_t)con.totallines) *
                    con.linewidth) +
                    _con.x
            ] = (uint32_t)c | mask;
            _con.x++;
            if (_con.x >= con.linewidth) {
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
        (Host_IsDedicated())
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

