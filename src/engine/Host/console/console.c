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

#include "types.h"
#include "console.h"
#include "common.h"
#include "z_hunk.h"
#include "enginedefs.h"
#include "cvar_q1.h"
#include "cmd.h"
#ifdef NeXT
#   include <libc.h>
#endif
#ifndef _MSC_VER
#   include <unistd.h>
#endif
#include <fcntl.h>
#include <stdio.h>
// #include <stdarg.h>
#include "VA.h"
#include <string.h>
#include "q_tools.h"
#include "client.h"
#include "sound.h"
#include "screen.h"
#include "host.h"

console_t con = {
    .cursorBlinkHz = 4
};

/*
    ================
    Con_Clear_f
    ================
*/
void Con_Clear_f() {
    if (con.pText) {
        Q_memset(con.pText, ' ', CON_TEXTSIZE);
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
    Con_Init
    ================
*/
#define MAXGAMEDIRLEN   (1000)
void Con_Init() {
    con.debuglog = COM_CheckParm("-condebug");

    if (con.debuglog) {
        cString t2 = "/qconsole.log";
        if (strlen(com.gamedir) < (MAXGAMEDIRLEN - strlen(t2))) {
            char temp[MAXGAMEDIRLEN + 1];
            snprintf(temp, sizeof(temp), "%s%s", com.gamedir, t2);
            unlink(temp);
        }
    }

    con.pText = Hunk_AllocName(CON_TEXTSIZE, "context");
    Q_memset(con.pText, ' ', CON_TEXTSIZE);
    con.linewidth = -1;
    Con_CheckResize();

    Con_Printf("Console initialized.\n");

    //
    // register our commands
    //
    Cvar_RegisterVariable(&con_notifytime);

    Cmd_AddCommand("clear", Con_Clear_f);
    con.isInitialized = true;
}


/*
    ===============
    Con_Linefeed
    ===============
*/
void Con_Linefeed() {
    con.x = 0;
    con.current++;
    Q_memset(
        &con.pText[(con.current % con.totallines) * con.linewidth],
        ' ',
        con.linewidth
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
    switch (txt[0]) {
    case 1: S_LocalSound("misc/talk.wav");  /* fall through */
    case 2: {
        mask = 0x80; /* 128 */
        txt++;
    } break;

    default: { mask = 0x00; } break;
    }

    char c;
    while ((c = *txt)) {
        // count word length
        int l = 0;
        for (; l < con.linewidth; l++) {
            if (txt[l] <= ' ')
                break;
        }

        // word wrap
        if ((l != con.linewidth) &&
            ((con.x + l) > (uint32_t)con.linewidth)) {
            con.x = 0;
        }
        txt++;

        if (cr) {
            con.current--;
            cr = false;
        }

        if (!con.x) {
            Con_Linefeed();
            // mark time for transparent overlay
            if (con.current >= 0)
                con.times[con.current % NUM_CON_TIMES] = GetRealTime();
        }

        switch (c) {
        case '\r':  cr = 1; /* fall through */
        case '\n':  con.x = 0;
            break;


        default: { // display character and advance
            con.pText[((con.current % con.totallines) * con.linewidth) + con.x] = c | mask;
            con.x++;
            if (con.x >= con.linewidth)
                con.x = 0;
        } break;
        }

    }
}


/*
    ================
    Con_DebugLog
    ================
*/
void Con_DebugLog(cString file, cString fmt, ...) {
    VaBuff_t data;
    VA_EXPAND(data, fmt);

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

    char msg[MAXPRINTMSG];
    VA_EXPAND(msg, fmt);
    // also echo to debugging console
    Host_Printf("%s", msg); // also echo to debugging console

    // log all messages to file
    if (con.debuglog)
        Con_DebugLog(va("%s/qconsole.log", com.gamedir), "%s", msg);


    if ((!con.isInitialized) ||
        (Host_IsDedicated())
        )
        return;  // no graphics mode


    // write it to the scrollable buffer
    Con_Print(msg);

    // update the screen if the console is displayed
    if ((cls.signon != SIGNONS) &&
        (!Scr.disabled_for_loading)
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

    char msg[MAXPRINTMSG];
    VA_EXPAND(msg, fmt);
    Con_Printf("%s", msg);
}


/*
    ==================
    Con_SafePrintf

    Okay to call even when the screen can't be updated
    ==================
*/
void Con_SafePrintf(cStringRO fmt, ...) {
    VaBuff_t msg;
    VA_EXPAND(msg, fmt);

    int temp = Scr.disabled_for_loading; {
        Scr.disabled_for_loading = true;
        Con_Printf("%s", msg);
    } Scr.disabled_for_loading = temp;
}



