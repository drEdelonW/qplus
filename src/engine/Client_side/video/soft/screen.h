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
// screen.h
#include "types.h"
#include "vid.h"
#include "qTime.h"

#ifdef __cplusplus
extern "C" {
#endif

    void SCR_Init();

    void SCR_UpdateScreen();
    void SCR_UpdateWholeScreen();

    void SCR_SizeUp();
    void SCR_SizeDown();
    void SCR_BringDownConsole();
    void SCR_CenterPrint(cString str);

    void SCR_BeginLoadingPlaque();
    void SCR_EndLoadingPlaque();

    int  SCR_ModalMessage(cString text);

#ifdef __cplusplus
}
#endif
// only the refresh window will be updated unless these variables are flagged
typedef struct {
    int      copytop;
    int      copyeverything;
    float    con_current;
    float    conlines;  // lines of console to display
    int      fullupdate; // set to 0 to force full redraw
    vRect_t  vrect;
    bool     disabled_for_loading;
    bool     skipupdate;
    LegacyTimeDelta_t    centertime_off;
} Screen_t;
extern Screen_t scr;

extern int      clearnotify; // set to 0 whenever notify text is drawn
extern bool     block_drawing;

