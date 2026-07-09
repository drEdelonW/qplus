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
// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "screen.h"
#include <string.h>
#include "cvar_q1.h"
#include "client.h"
#include "draw.h"
#include "host.h"
#include "sys.h"
#include <math.h>
#include "sbar.h"
#include "console.h"
#include "common.h"
#include "d_iface.h"
#include "sound.h"
// #include "menu.h"
#include "render.h"
#include "view.h"

#include "screen_prv.h"

#if 0
static vRect_p  _pConUpdate;
#endif

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/
#include "vid.h" // vid.numpages
void SCR_EraseCenterString() {
    if (_scr.erase_center++ > vid.numpages) {
        _scr.erase_lines = 0;
        return;
    }

    int y = (_scr.center_lines <= 4) ?
        vid.scr.height * 0.35 : 48;

    Scr.copytop = true;
    Draw_TileClear(0, y, vid.scr.width, 8 * _scr.erase_lines);
}

//=============================================================================

/*
==============================================================================

                        SCREEN SHOTS

==============================================================================
*/

#include "pcx.h"

/*
==================
SCR_ScreenShot_f
==================
*/
#include "vid.h" // vid.scr.pBuff
void SCR_ScreenShot_f() {
    //
    // find a file name to save it to
    //
    char pcxname[80];
    strcpy(pcxname, "quake00.pcx");
    // -- pcxname[5/6] is ^^ this positions
    for (int i = 0; i <= 99; i++) {
        pcxname[5] = (i / 10) + '0';
        pcxname[6] = (i % 10) + '0';

        char checkname[MAX_OSPATH];
        snprintf(checkname, sizeof(checkname), "%s/%s", com.gamedir, pcxname);
        if (Sys_FileTime(checkname) == -1) {     // save the pcx file
            D_EnableBackBufferAccess(); // enable direct drawing of console to back
            //  buffer

            WritePCXfile(
                pcxname, vid.scr.pBuff,
                vid.scr.width, vid.scr.height,
                vid.rowbytes, host_basepal
            );

            D_DisableBackBufferAccess(); // for adapters that can't stay mapped in
            //  for linear writes all the time

            Con_Printf("Wrote %s\n", pcxname);
        }
    }
    Con_Printf("SCR_ScreenShot_f: Couldn't create a PCX file\n");
}


//=============================================================================



/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen() {
    if (Scr.block_drawing ||
        Scr.skipupdate)
        return;

    Scr.copytop = false;        // TODO: wrap this valuse to avoid global publishing
    Scr.copyeverything = false; // TODO: wrap this valuse to avoid global publishing

    if (Scr.disabled_for_loading) {
        if ((GetRealTime() - _scr.disabled_time) > 60) {
            Scr.disabled_for_loading = false;
            Con_Printf("load failed.\n");
        }
        else    return;
    }

    if (Host_IsDedicated())     return; // stdout only

    if (!_scr.initialized || !con.isInitialized)
        return;     // not initialized yet

    //
    // check for vid changes
    //
#ifndef STM32
    static float _oldLcdX;
    if (_oldLcdX != lcd_x.value) {
        _oldLcdX = lcd_x.value;
        SCR_RequestCalcRefdef();
    }
#endif

    if (_scr.oldFov != scr_fov.value) {
        _scr.oldFov = scr_fov.value;
        SCR_RequestCalcRefdef();
    }

    if (_scr.oldViewSize != scr_viewsize.value) {
        _scr.oldViewSize = scr_viewsize.value;
        SCR_RequestCalcRefdef();
    }

    SCR_CalcRefdef();

    //
    // do 3D refresh drawing, and then update the screen
    //
    D_EnableBackBufferAccess(); // of all overlay stuff if drawing directly

    if (fullupdate++ < vid.numpages) { // clear the entire screen
        Scr.copyeverything = true;
        Draw_TileClear(0, 0, vid.scr.width, vid.scr.height);
        Sbar_Changed();
    }

#if 0
    _pConUpdate = NULL;
#endif

    SCR_SetUpToDrawConsole();
    SCR_EraseCenterString();

    D_DisableBackBufferAccess(); // for adapters that can't stay mapped in
    //  for linear writes all the time

    VID_LockBuffer();
    V_RenderView();
    VID_UnlockBuffer();

    D_EnableBackBufferAccess(); { // of all overlay stuff if drawing directly
        SCR_Composite();    // main state based compositor
    } D_DisableBackBufferAccess(); // for adapters that can't stay mapped in
    //  for linear writes all the time
#if 0
    if (_pConUpdate)
        D_UpdateRects(_pConUpdate);
#endif

    V_UpdatePalette();

    //
    // update one of three areas
    //

    vRect_t  vrect;
    if (Scr.copyeverything) {   // fullScreen viewport withOUT sBar
        vrect = (vRect_t){
            .x = 0,
            .y = 0,
            .width = vid.scr.width,
            .height = vid.scr.height,
        };
    }
    else if (Scr.copytop) {     // fullScreen viewport with sBar
        vrect = (vRect_t){
            .x = 0,
            .y = 0,
            .width = vid.scr.width,
            .height = vid.scr.height - sb_lines,
        };
    }
    else {                      // center screen rectangle viewport with sBar
        vrect = (vRect_t){
            .x = Scr.vrect.x,
            .y = Scr.vrect.y,
            .width = vid.scr.width,
            .height = vid.scr.height,
        };
    }
    VID_Update(&vrect);
}


/*
==================
SCR_UpdateWholeScreen
==================
*/
void SCR_UpdateWholeScreen() {
    SCR_RequestRedraw();
    SCR_UpdateScreen();
}
