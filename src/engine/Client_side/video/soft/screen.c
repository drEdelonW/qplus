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
#include "menu.h"
#include "render.h"

#include "screen_prv.h"

static vRect_p  _pConUpdate;

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

void SCR_EraseCenterString() {
    if (_scr.erase_center++ > vid.numpages) {
        _scr.erase_lines = 0;
        return;
    }

    int y = (_scr.center_lines <= 4) ?
        vid.height * 0.35 : 48;

    scr.copytop = true;
    Draw_TileClear(0, y, vid.width, 8 * _scr.erase_lines);
}

//=============================================================================



/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef() {
    scr.fullupdate = 0;  // force a background redraw
    vid.recalc_refdef = 0;

    // force the status bar to redraw
    Sbar_Changed();

    //========================================

    // bound viewsize
    if (scr_viewsize.value < 30)    Cvar_Set("viewsize", "30");
    if (scr_viewsize.value > 120)   Cvar_Set("viewsize", "120");

    // bound field of view
    if (scr_fov.value < 10)         Cvar_Set("fov", "10");
    if (scr_fov.value > 170)        Cvar_Set("fov", "170");

    r_refdef.fov_x = scr_fov.value;
    r_refdef.fov_y = CalcFov(r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

    // intermission is always full screen
    float  size;
    if (cl.intermission != IM_NONE) size = 120;
    else                            size = scr_viewsize.value;

    if (size >= 120)        sb_lines = 0;  // no status bar at all
    else if (size >= 110)   sb_lines = 24;  // no inventory
    else                    sb_lines = 24 + 16 + 8;

    // these calculations mirror those in R_Init() for r_refdef, but take no
    // account of water warping
    vRect_t vrect = {
        .width = vid.width,
        .height = vid.height
    };

    R_SetVrect(&vrect, &scr.vrect, sb_lines);

    // guard against going from one mode to another that's less than half the
    // vertical resolution
    if (scr.con_current > vid.height)
        scr.con_current = vid.height;

    // notify the refresh of the change
    R_ViewChanged(&vrect, sb_lines, vid.aspect);
}


//============================================================================

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
void SCR_ScreenShot_f() {
    //
    // find a file name to save it to
    //
    char pcxname[80]; strcpy(pcxname, "quake00.pcx");

    for (int i = 0; i <= 99; i++) {
        pcxname[5] = (i / 10) + '0';
        pcxname[6] = (i % 10) + '0';

        char checkname[MAX_OSPATH];
        snprintf(checkname, sizeof(checkname), "%s/%s", com.gamedir, pcxname);
        if (Sys_FileTime(checkname) == -1) {     // save the pcx file
            D_EnableBackBufferAccess(); // enable direct drawing of console to back
            //  buffer

            WritePCXfile(
                pcxname, vid.buffer,
                vid.width, vid.height,
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
    static float _oldLcdX;

    if (scr.skipupdate || scr.block_drawing)
        return;

    scr.copytop = false;
    scr.copyeverything = false;

    if (scr.disabled_for_loading) {
        if (realtime - _scr.disabled_time > 60) {
            scr.disabled_for_loading = false;
            Con_Printf("load failed.\n");
        }
        else    return;
    }

    if (Host_IsDedicated())
        return;    // stdout only

    if (!_scr.initialized || !con.isInitialized)
        return;    // not initialized yet

    if (_scr.oldScrViewSize != scr_viewsize.value) {
        _scr.oldScrViewSize = scr_viewsize.value;
        vid.recalc_refdef = 1;
    }

    //
    // check for vid changes
    //
    if (_scr.oldFov != scr_fov.value) {
        _scr.oldFov = scr_fov.value;
        vid.recalc_refdef = true;
    }

    if (_oldLcdX != lcd_x.value) {
        _oldLcdX = lcd_x.value;
        vid.recalc_refdef = true;
    }

    if (_scr.oldScrViewSize != scr_viewsize.value) {
        _scr.oldScrViewSize = scr_viewsize.value;
        vid.recalc_refdef = true;
    }

    if (vid.recalc_refdef) {
        // something changed, so reorder the screen
        SCR_CalcRefdef();
    }

    //
    // do 3D refresh drawing, and then update the screen
    //
    D_EnableBackBufferAccess(); // of all overlay stuff if drawing directly

    if (scr.fullupdate++ < vid.numpages) { // clear the entire screen
        scr.copyeverything = true;
        Draw_TileClear(0, 0, vid.width, vid.height);
        Sbar_Changed();
    }

    _pConUpdate = NULL;


    SCR_SetUpToDrawConsole();
    SCR_EraseCenterString();

    D_DisableBackBufferAccess(); // for adapters that can't stay mapped in
    //  for linear writes all the time

    VID_LockBuffer();
    V_RenderView();
    VID_UnlockBuffer();

    D_EnableBackBufferAccess(); { // of all overlay stuff if drawing directly

        if (_scr.drawdialog) {
            Sbar_Draw();
            Draw_FadeScreen();
            SCR_DrawNotifyString();
            scr.copyeverything = true;
        }
        else if (_scr.drawloading) {
            SCR_DrawLoading();
            Sbar_Draw();
        }
        else if ((cl.intermission == IM_LEVEL) && (key.dest == key_game)) {
            Sbar_IntermissionOverlay();
        }
        else if ((cl.intermission == IM_FINALE) && (key.dest == key_game)) {
            Sbar_FinaleOverlay();
            SCR_CheckDrawCenterString();
        }
        else if ((cl.intermission == IM_CUTSCENE) && (key.dest == key_game)) {
            SCR_CheckDrawCenterString();
        }
        else {
            SCR_DrawRam();
            SCR_DrawNet();
            SCR_DrawTurtle();
            SCR_DrawPause();
            SCR_CheckDrawCenterString();
            Sbar_Draw();
            SCR_DrawConsole();
            M_Draw();
        }

    } D_DisableBackBufferAccess(); // for adapters that can't stay mapped in
    //  for linear writes all the time
    if (_pConUpdate)
        D_UpdateRects(_pConUpdate);


    V_UpdatePalette();

    //
    // update one of three areas
    //

    vRect_t  vrect;
    if (scr.copyeverything) {   // fullScreen viewport withOUT sBar
        vrect = (vRect_t){
            .x = 0,
            .y = 0,
            .width = vid.width,
            .height = vid.height,
            .pnext = 0
        };
    }
    else if (scr.copytop) {     // fullScreen viewport with sBar
        vrect = (vRect_t){
            .x = 0,
            .y = 0,
            .width = vid.width,
            .height = vid.height - sb_lines,
            .pnext = 0
        };
    }
    else {                      // center screen rectangle viewport with sBar
        vrect = (vRect_t){
            .x = scr.vrect.x,
            .y = scr.vrect.y,
            .width = vid.width,
            .height = vid.height,
            .pnext = 0
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
    scr.fullupdate = 0;
    SCR_UpdateScreen();
}
