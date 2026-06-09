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
#include "qPic.h"
#include <string.h>
#include "cvar_q1.h"
#include "client.h"
#include "draw.h"
#include "host.h"
#include "sys.h"
#include <math.h>
#include "sbar.h"
#include "cmd.h"
#include "console.h"
#include "common.h"
#include "d_iface.h"
#include "sound.h"
#include "menu.h"

Screen_t scr;

int     clearnotify;
bool    block_drawing;


typedef struct {
    bool     initialized;  // ready to draw
    qPic_p   ram;
    qPic_p   net;
    qPic_p   turtle;
    bool     drawloading;
    bool     drawdialog;
    int      erase_lines;
    int      erase_center;
    cString  notifystring;
    LegacyTimeDelta_t    disabled_time;
    LegacyTimeDelta_t    centertime_start; // for slow victory printing
    int      center_lines;
    char     centerstring[1024];
} _Screen_t;
static _Screen_t _scr;

static float    _oldScreenSize, _oldFov;
static int      _clearConsole;
static vRect_p  _pConUpdate;

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint(cString str) {
    strncpy(_scr.centerstring, str, sizeof(_scr.centerstring) - 1);
    scr.centertime_off = scr_centertime.value;
    _scr.centertime_start = cl.time;

    // count the number of lines for centering
    _scr.center_lines = 1;
    while (*str) {
        if (*str == '\n')
            _scr.center_lines++;
        str++;
    }
}

void SCR_EraseCenterString() {
    if (_scr.erase_center++ > vid.numpages) {
        _scr.erase_lines = 0;
        return;
    }

    int y = (_scr.center_lines <= 4) ?
        vid.height * 0.35 : 48;

    scr.copytop = 1;
    Draw_TileClear(0, y, vid.width, 8 * _scr.erase_lines);
}

void SCR_DrawCenterString() {
    // the finale prints the characters one at a time
    int remaining = (cl.intermission != IM_NONE) ?
        scr_printspeed.value * (cl.time - _scr.centertime_start) : 9999;

    _scr.erase_center = 0;
    cString start = _scr.centerstring;

    int y = (_scr.center_lines <= 4) ?
        vid.height * 0.35 : 48;

    do {
        // scan the width of the line
        int inLine = 0;
        for (; inLine < 40; inLine++)
            if ((start[inLine] == '\n') || !start[inLine])
                break;

        int x = (vid.width - inLine * 8) / 2;
        for (int j = 0; j < inLine; j++, x += 8) {
            Draw_Character(x, y, start[j]);
            if (!remaining--)
                return;
        }

        y += 8;

        while (*start && *start != '\n')
            start++;

        if (!*start)
            break;
        start++;  // skip the \n
    } while (1);
}

void SCR_CheckDrawCenterString() {
    scr.copytop = 1;
    if (_scr.center_lines > _scr.erase_lines)
        _scr.erase_lines = _scr.center_lines;

    scr.centertime_off -= host_frametime;

    if (((scr.centertime_off <= 0) &&
        (cl.intermission == IM_NONE)) ||
        (key.dest != key_game)
        )
        return;

    SCR_DrawCenterString();
}

//=============================================================================

/*
====================
CalcFov
====================
*/
float CalcFov(float fov_x, float width, float height) {
    if ((fov_x < 1) ||
        (fov_x > 179))
        Host_SysError("Bad fov: %f", fov_x);

    float x = width / tan(fov_x / 360 * M_PI);
    float at = atan(height / x);

    at = at * 360 / M_PI;

    return at;
}

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


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f() {
    Cvar_SetValue("viewsize", scr_viewsize.value + 10);
    vid.recalc_refdef = 1;
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f() {
    Cvar_SetValue("viewsize", scr_viewsize.value - 10);
    vid.recalc_refdef = 1;
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


/*
==================
SCR_Init
==================
*/
void SCR_Init() {
    Cvar_RegisterVariable(&scr_fov);
    Cvar_RegisterVariable(&scr_viewsize);
    Cvar_RegisterVariable(&scr_conspeed);
    Cvar_RegisterVariable(&scr_showram);
    Cvar_RegisterVariable(&scr_showturtle);
    Cvar_RegisterVariable(&scr_showpause);
    Cvar_RegisterVariable(&scr_centertime);
    Cvar_RegisterVariable(&scr_printspeed);

    //
    // register our commands
    //
    Cmd_AddCommand("screenshot", SCR_ScreenShot_f);
    Cmd_AddCommand("sizeup", SCR_SizeUp_f);
    Cmd_AddCommand("sizedown", SCR_SizeDown_f);

    _scr.ram = Draw_PicFromWad("ram");
    _scr.net = Draw_PicFromWad("net");
    _scr.turtle = Draw_PicFromWad("turtle");

    _scr.initialized = true;
}



/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam() {
    if ((!scr_showram.value) ||
        (!r_cache_thrash))
        return;
    // printf("drawRAM [%s]  \n", r_cache_thrash ? "true" : "false");
    Draw_Pic(scr.vrect.x + 32, scr.vrect.y, _scr.ram);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle() {
    static int _cnt;

    if (!scr_showturtle.value)  return;

    if (host_frametime < 0.1) { _cnt = 0;  return; }

    _cnt++;
    if (_cnt < 3)  return;

    Draw_Pic(scr.vrect.x, scr.vrect.y, _scr.turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet() {
    if ((realtime - cl.last_received_message < 0.3) ||
        (cls.demoplayback))
        return;

    Draw_Pic(scr.vrect.x + 64, scr.vrect.y, _scr.net);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause() {
    if ((!scr_showpause.value) ||  // turn off for screenshots
        (!cl.paused))
        return;

    qPic_p pic = Draw_CachePic("gfx/pause.lmp");
    Draw_Pic((vid.width - pic->width) / 2,
        (vid.height - 48 - pic->height) / 2, pic);
}



/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading() {
    if (!_scr.drawloading)   return;

    qPic_p pic = Draw_CachePic("gfx/loading.lmp");
    Draw_Pic((vid.width - pic->width) / 2,
        (vid.height - 48 - pic->height) / 2, pic);
}



//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole() {
    Con_CheckResize();

    if (_scr.drawloading)    return;  // never a console with loading plaque

    // decide on the height of the console
    con.forcedup = !cl.worldmodel || cls.signon != SIGNONS;

    if (con.forcedup) {
        scr.conlines = vid.height;  // full screen
        scr.con_current = scr.conlines;
    }
    else if (key.dest == key_console)   scr.conlines = vid.height / 2; // half screen
    else                                scr.conlines = 0;    // none visible

    if (scr.conlines < scr.con_current) {
        scr.con_current -= scr_conspeed.value * host_frametime;
        if (scr.conlines > scr.con_current)
            scr.con_current = scr.conlines;

    }
    else if (scr.conlines > scr.con_current) {
        scr.con_current += scr_conspeed.value * host_frametime;
        if (scr.conlines < scr.con_current)
            scr.con_current = scr.conlines;
    }

    if (_clearConsole++ < vid.numpages) {
        scr.copytop = 1;
        Draw_TileClear(0, (int)scr.con_current, vid.width, vid.height - (int)scr.con_current);
        Sbar_Changed();
    }
    else if (clearnotify++ < vid.numpages) {
        scr.copytop = 1;
        Draw_TileClear(0, 0, vid.width, con.notifylines);
    }
    else
        con.notifylines = 0;
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole() {
    if (scr.con_current) {
        scr.copyeverything = 1;
        Con_DrawConsole(scr.con_current, true);
        _clearConsole = 0;
    }
    else {
        if ((key.dest == key_game) || (key.dest == key_message))
            Con_DrawNotify(); // only draw notify in game
    }
}


//=============================================================================


/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque() {
    S_StopAllSounds(true);

    if ((cls.state != ca_connected) ||
        (cls.signon != SIGNONS))
        return;

    // redraw with no console and the loading plaque
    Con_ClearNotify();
    scr.centertime_off = 0;
    scr.con_current = 0;

    _scr.drawloading = true;
    scr.fullupdate = 0;
    Sbar_Changed();
    SCR_UpdateScreen();
    _scr.drawloading = false;

    scr.disabled_for_loading = true;
    _scr.disabled_time = realtime;
    scr.fullupdate = 0;
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque() {
    scr.disabled_for_loading = false;
    scr.fullupdate = 0;
    Con_ClearNotify();
}

//=============================================================================



void SCR_DrawNotifyString() {
    cString start = _scr.notifystring;
    int y = vid.height * 0.35;

    do {
        // scan the width of the line
        int inLine = 0;
        for (; inLine < 40; inLine++)
            if ((start[inLine] == '\n') || !start[inLine])    break;

        int x = (vid.width - inLine * 8) / 2;
        for (int j = 0; j < inLine; j++, x += 8)
            Draw_Character(x, y, start[j]);

        y += 8;

        while (*start && *start != '\n')
            start++;

        if (!*start)    break;
        start++;  // skip the \n
    } while (1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
int SCR_ModalMessage(cString text) {
    if (Host_IsDedicated())  return true;

    _scr.notifystring = text;

    // draw a fresh screen
    scr.fullupdate = 0;
    _scr.drawdialog = true;
    SCR_UpdateScreen();
    _scr.drawdialog = false;

    S_ClearBuffer();  // so dma doesn't loop current sound

    do {
        key.count = -1;  // wait for a key down and up
        Sys_SendKeyEvents();
    } while (
        (key.lastpress != 'y') &&
        (key.lastpress != 'n') &&
        (key.lastpress != K_ESCAPE));

    scr.fullupdate = 0;
    SCR_UpdateScreen();

    return key.lastpress == 'y';
}


//=============================================================================

/*
===============
SCR_BringDownConsole

Brings the console down and fades the palettes back to normal
================
*/
void SCR_BringDownConsole() {
    scr.centertime_off = 0;

    for (int i = 0; (i < 20) && (scr.conlines != scr.con_current); i++)
        SCR_UpdateScreen();

    cl.cshifts[0].percent = 0;  // no area contents palette on next frame
    VID_SetPalette(host_basepal);
}


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
    static float _oldScrViewSize;
    static float _oldLcdX;

    if (scr.skipupdate || block_drawing)
        return;

    scr.copytop = 0;
    scr.copyeverything = 0;

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

    if (scr_viewsize.value != _oldScrViewSize) {
        _oldScrViewSize = scr_viewsize.value;
        vid.recalc_refdef = 1;
    }

    //
    // check for vid changes
    //
    if (_oldFov != scr_fov.value) {
        _oldFov = scr_fov.value;
        vid.recalc_refdef = true;
    }

    if (_oldLcdX != lcd_x.value) {
        _oldLcdX = lcd_x.value;
        vid.recalc_refdef = true;
    }

    if (_oldScreenSize != scr_viewsize.value) {
        _oldScreenSize = scr_viewsize.value;
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
        scr.copyeverything = 1;
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

    if (scr.copyeverything) {
        vRect_t  vrect = {
            .x = 0,
            .y = 0,
            .width = vid.width,
            .height = vid.height,
            .pnext = 0
        };

        VID_Update(&vrect);
    }
    else if (scr.copytop) {
        vRect_t  vrect = {
            .x = 0,
            .y = 0,
            .width = vid.width,
            .height = vid.height - sb_lines,
            .pnext = 0
        };

        VID_Update(&vrect);
    }
    else {
        vRect_t  vrect = {
            .x = scr.vrect.x,
            .y = scr.vrect.y,
            .width = vid.width,
            .height = vid.height,
            .pnext = 0
        };

        VID_Update(&vrect);
    }
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
