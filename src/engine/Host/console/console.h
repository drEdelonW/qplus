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

#define CON_HORIZONLINE "\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n"

//
// console
//
#define CON_TEXTSIZE    (int32_t)0x4000 /*16Kb - 16384b*/

#define NUM_CON_TIMES (4)
#define MAXCHATLEN  (32)
#define MAXCMDLINE  (256)
typedef struct {
    bool    isInitialized;
    char    lines[MAXCHATLEN][MAXCMDLINE];
    sRealTime_t times[NUM_CON_TIMES]; // realtime time the line was generated for transparent notify lines
    int32_t totallines; // total lines in console scrollback
    int32_t backscroll; // lines up from bottom to display
    int32_t notifylines;// scan lines to clear for notify lines
    int32_t vislines;
    int32_t linewidth;
    int32_t current;    // where next message will be printed
    int32_t edit_line;
    uint32_t linepos;
    uint32_t x;         // offset in current line for next print
    float   cursorspeed;
    cString text;
    bool    forcedup;   // because no entities to refresh
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