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
 * vid      (platform specific graphic environment) <- "vid.h" (vid is for SoftRender)
 * └── screen   (layer compositor)                  <- "Screen.h"
 *     ├── render viewport  (back layer)            <- "render.h", "View.h"
 *     ├── HUD              (overlay viewport)      <- "hud.h"
 *     │   ├── sbar         (bottom)                <- "sbar.h"
 *     │   ├── crosshair    (center of viewport)
 *     │   ├── center msg   (center message)
 *     │   ├── status msgs  (top left corner)
 *     │   └── sys icons    (top right corner)
 *     ├── menu            (front layer)            <- "menu.h"
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
#include "qTime.h"
#include "vRect.h"
#include "qColor.h"
#include "qLight.h"
#include "hud.h"
#include "view.h"

#define BASEWIDTH  (320)
#define BASEHEIGHT (200)

 // only the refresh window will be updated unless these variables are flagged
typedef struct {
    vRect_t canvas;     // Whole screen size rectangle
    ptrdiff_t SR_rowBytes;  // Width in bytes - new line offset // TODO: move it to SoftRender specific

    qColor8_p direct;   // direct drawing to framebuffer, if not NULL
    int numpages;

    vRect_t con;        // Console size rectangle
    int con_current;
    int conlines;       // lines of console to display

    ColorMap_p  pColorMapPal;   // 256 * VID_GRADES size   
#if 1 /* TODO: not useful? */
    qColor16_p  pColorMap16;     // 256 * VID_GRADES size // TODO: check is ot not used?
#endif

    int clearnotify;    // set to 0 whenever notify text is drawn
#if 1   /* this specific for software render. not applicable for OpenGL */
    bool    copytop;
    bool    copyeverything;     // software frame buffer copy request
#endif
    bool    disabled_for_loading;
    bool    skipupdate;
    bool    block_drawing;
    bool    r_cache_thrash;     // compatability
    float   vpAspect;           // width / height -- < 0 is taller than wide  // move to view.h or RefDef
} Screen_t;
extern Screen_t Scr;


static inline float calcAspect(int width, int height) { return ((float)height / (float)width) * (320.0 / 240.0); }
static inline float calcAspectRect(vRect_p vR) { return ((float)vR->height / (float)vR->width) * (320.0 / 240.0); }

#ifdef __cplusplus
extern "C" {
#endif

    void SCR_Init();
    void SCR_UpdateScreen();
    void SCR_RequestRedraw();
    void SCR_InstantUpdateScreen();   // INFO: Win VID specific
    void SCR_CenterPrint(cString str);
    void SCR_BeginLoadingPlaque();
    void SCR_EndLoadingPlaque();
    int  SCR_ModalMessage(cString text);
    void Con_CheckResize();
    void Con_ToggleConsole_f();
    void Draw_ConsoleBackground(int lines);

    void HideCenterPrint();

#ifdef __cplusplus
}
#endif



