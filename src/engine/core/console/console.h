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

#define CON_HORIZONLINE "\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n"

//
// console
//
extern int32_t con_totallines;
extern int32_t con_backscroll;
extern bool con_forcedup; // because no entities to refresh
extern bool con_initialized;
// extern uint8_p con_chars;
extern int32_t con_notifylines;  // scan lines to clear for notify lines
#ifdef __cplusplus
extern "C" {
#endif
    void Con_DrawCharacter(int32_t cx, int32_t line, int32_t num);

    void Con_CheckResize();
    void Con_Init();
    void Con_DrawConsole(int32_t lines, bool drawinput);
    void Con_Print(cString txt);
    void Con_Printf(cString fmt, ...);
    void Con_DPrintf(cString fmt, ...);
    void Con_SafePrintf(cString fmt, ...);
    void Con_Clear_f();
    void Con_DrawNotify();
    void Con_ClearNotify();
    void Con_ToggleConsole_f();

    void Con_NotifyBox(cString text); // during startup for sound / cd warnings

#ifdef __cplusplus
}
#endif