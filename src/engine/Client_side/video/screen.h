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

/*  Screen layout concept
 * vid      (platform specific graphic environment)
 * └── screen   (layer compositor)
 *     ├── render viewport  (back layer)
 *     ├── HUD              (overlay viewport)
 *     │   ├── crosshair    (center of viewport)
 *     │   ├── sbar         (bottom)
 *     │   ├── status msgs  (top left corner)
 *     │   └── sys icons    (top right corner)
 *     ├── menu            (front layer)
 *     └── echo console    (top layer, full/half screen)
 * 
 *  +--echo console (full / half screen)-----------+
 *  +--menu----------------------------------------+
 *  +--HUD-----------------------------------------+
 *  |  [status msgs]                  [sys icons]  |
 *  |                   [+]                        |
 *  |               crosshair                      |
 *  |  [==================sbar==================]  |
 *  +--render viewport-----------------------------+
 *  |  3D scene - player POV                       |
 *  +----------------------------------------------+
 */

#include "types.h"
#include "vid.h"
#include "qTime.h"

// only the refresh window will be updated unless these variables are flagged
typedef struct {
    bool    copytop;
    bool    copyeverything;
    float   con_current;
    float   conlines;           // lines of console to display
    int     fullupdate;         // set to 0 to force full redraw
    vRect_t vrect;
    bool    disabled_for_loading;
    bool    skipupdate;
    LegacyTimeDelta_t    centertime_off;
    int     clearnotify;        // set to 0 whenever notify text is drawn
    bool    block_drawing;
    bool    r_cache_thrash;     // compatability
} Screen_t;
extern Screen_t scr;

#ifdef __cplusplus
extern "C" {
#endif

    void SCR_Init();

    void SCR_UpdateScreen();
    void SCR_UpdateWholeScreen();   // INFO: Win VID specific

    void SCR_CenterPrint(cString str);

    void SCR_BeginLoadingPlaque();
    void SCR_EndLoadingPlaque();

    int  SCR_ModalMessage(cString text);

#ifdef __cplusplus
}
#endif



