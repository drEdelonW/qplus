#pragma once
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
#include "types.h"
#include "qTime.h"



//
// console
//
#define CON_TEXTSIZE  (0x4000) /*16Kb - 16384b*/
#define NUM_CON_TIMES (4)

typedef enum X {
    FirstChar = 0,
    LastChar = 31,
    MaxCharLen // 32
} CharCol_t;    /* X */
// #define MAXCHATLEN  (32)

typedef enum Y {
    FirstLine = 0,
    LastLine = 255,
    MaxCmdLine // 256
} CmdLine_t;    /* Y */
// #define MAXCMDLINE  (256)

typedef struct {
    bool    isInitialized;

    char    lines[MaxCharLen][MaxCmdLine];
    sRealTime_t times[NUM_CON_TIMES]; // realtime time the line was generated for transparent notify lines
    cString pText;

    CmdLine_t   edit_line;
    CmdLine_t   current;    // where next message will be printed
    CmdLine_t   backscroll; // lines up from bottom to display
    CmdLine_t   totallines; // total lines in console scrollback
    CmdLine_t   vislines;   // internal
    CmdLine_t   notifylines;// scan lines to clear for notify lines

    CharCol_t   linewidth;
    CharCol_t   linepos;
    CharCol_t   x;          // offset in current line for next print

    float   cursorBlinkHz;  // cursor blink speed [tenses per second]
    bool    forcedup;       // because no entities to refresh
    bool    debuglog;
} console_t;
extern console_t con;


#ifdef __cplusplus
extern "C" {
#endif

    void Con_Init();
    void Con_Print(cStringRO txt);
    void Con_Printf(cStringRO fmt, ...);
    void Con_DPrintf(cStringRO fmt, ...);
    void Con_SafePrintf(cStringRO fmt, ...);
    void Con_Clear_f();
    void Con_ClearNotify();

#ifdef __cplusplus
}
#endif