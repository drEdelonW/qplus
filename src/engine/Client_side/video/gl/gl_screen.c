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


/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint()
SlowPrint()
Screen_Update();
Con_Printf();

net
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
    notify lines
    half
    full


*/
#include "qOpenGL.h"
#include "screen.h"
#include "cvar.h"
#include "vid.h"
#include <string.h>
#include "client.h"
#include "host.h"
#include "draw.h"
#include "mathlib.h"
#include "sbar.h"
#include "cmd.h"
#include "console.h"
#include "common.h"
#include "sys.h"
#include "sound.h"
#include "menu.h"
#include "cvar_q1.h"
#include <stdlib.h>

#include "screen_prv.h"

int glx, gly, glwidth, glheight; // extern
cvar_t  gl_triplebuffer = { "gl_triplebuffer", "1", true };

int     scr_fullupdate;     // TODO: check is it needed to vid_win.c as extern
vRect_t     scr_vrect;  // extern vRect_t scr_vrect; //
bool    scr_disabled_for_loading;

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef() {
    scr_fullupdate = 0;        // force a background redraw
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

    // intermission is always full screen
    float   size;
    if (cl.intermission != IM_NONE)     size = 120;
    else                                size = scr_viewsize.value;

    /**/ if (size >= 120.0f)    sb_lines = 0;        // no status bar at all
    else if (size >= 110.0f)    sb_lines = 24;       // no inventory
    else /*               */    sb_lines = 24 + 16 + 8;

    bool    full = false;
    if (scr_viewsize.value >= 100.0f) {
        full = true;
        size = 100.0f;
    }
    else
        size = scr_viewsize.value;
    if (cl.intermission != IM_NONE) {
        full = true;
        size = 100.0f;
        sb_lines = 0;
    }
    size /= 100.0f;

    int h = vid.height - sb_lines;

    r_refdef.vrect.width = vid.width * size;
    if (r_refdef.vrect.width < 96) {
        size = 96.0 / r_refdef.vrect.width;
        r_refdef.vrect.width = 96;    // min for icons
    }

    r_refdef.vrect.height = vid.height * size;
    if (r_refdef.vrect.height > (vid.height - sb_lines))
        r_refdef.vrect.height = (vid.height - sb_lines);
    if (r_refdef.vrect.height > vid.height)
        r_refdef.vrect.height = vid.height;
    r_refdef.vrect.x = (vid.width - r_refdef.vrect.width) / 2;
    if (full)   r_refdef.vrect.y = 0;
    else        r_refdef.vrect.y = (h - r_refdef.vrect.height) / 2;

    r_refdef.fov_x = scr_fov.value;
    r_refdef.fov_y = CalcFov(r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

    scr_vrect = r_refdef.vrect;
}


/*
==============================================================================

                        SCREEN SHOTS

==============================================================================
*/

typedef struct _TargaHeader {
    uint8_t     id_length, colormap_type, image_type;
    uint16_t    colormap_index, colormap_length;
    uint8_t     colormap_size;
    uint16_t    x_origin, y_origin, width, height;
    uint8_t     pixel_size, attributes;
} TargaHeader;


/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f() {
    //
    // find a file name to save it to
    //
    char pcxname[80];
    strcpy(pcxname, "quake00.tga");

    int i = 0;
    for (; i <= 99; i++) {
        pcxname[5] = i / 10 + '0';
        pcxname[6] = i % 10 + '0';
        char checkname[MAX_OSPATH];
        snprintf(checkname, sizeof(checkname), "%s/%s", com.gamedir, pcxname);
        if (Sys_FileTime(checkname) == -1)
            break;    // file doesn't exist
    }
    if (i == 100) {
        Con_Printf("SCR_ScreenShot_f: Couldn't create a PCX file\n");
        return;
    }


    uint8_p buffer = malloc(glwidth * glheight * 3 + 18);
    memset(buffer, 0, 18);
    buffer[2] = 2;        // uncompressed type
    buffer[12] = glwidth & 255;
    buffer[13] = glwidth >> 8;
    buffer[14] = glheight & 255;
    buffer[15] = glheight >> 8;
    buffer[16] = 24;    // pixel size

    glReadPixels(
        glx, gly,
        glwidth, glheight,
        GL_RGB, GL_UNSIGNED_BYTE,
        buffer + 18
    );

    // swap rgb to bgr
    int c = 18 + glwidth * glheight * 3;
    for (int i = 18; i < c; i += 3) {
        int temp = buffer[i];
        buffer[i] = buffer[i + 2];
        buffer[i + 2] = temp;
    }
    COM_WriteFile(pcxname, buffer, glwidth * glheight * 3 + 18);

    free(buffer);
    Con_Printf("Wrote %s\n", pcxname);
}


//=============================================================================

void SCR_TileClear() {
    if (r_refdef.vrect.x > 0) {
        // left
        Draw_TileClear(
            0,
            0,
            r_refdef.vrect.x,
            vid.height - sb_lines
        );
        // right
        Draw_TileClear(
            r_refdef.vrect.x + r_refdef.vrect.width,
            0,
            vid.width - r_refdef.vrect.x + r_refdef.vrect.width,
            vid.height - sb_lines
        );
    }
    if (r_refdef.vrect.y > 0) {
        // top
        Draw_TileClear(
            r_refdef.vrect.x,
            0,
            r_refdef.vrect.x + r_refdef.vrect.width,
            r_refdef.vrect.y
        );
        // bottom
        Draw_TileClear(
            r_refdef.vrect.x,
            r_refdef.vrect.y + r_refdef.vrect.height,
            r_refdef.vrect.width,
            vid.height - sb_lines - (r_refdef.vrect.height + r_refdef.vrect.y)
        );
    }
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
    if (scr.block_drawing)
        return;

    vid.numpages = 2 + gl_triplebuffer.value;

    scr.copytop = false;
    scr.copyeverything = false;

    if (scr_disabled_for_loading) {
        if (realtime - _scr.disabled_time > 60) {
            scr_disabled_for_loading = false;
            Con_Printf("load failed.\n");
        }
        else    return;
    }

    if (!_scr.initialized || !con.isInitialized)
        return;                // not initialized yet


    GL_BeginRendering(&glx, &gly, &glwidth, &glheight); {

        //
        // determine size of refresh window
        //
        if (_scr.oldFov != scr_fov.value) {
            _scr.oldFov = scr_fov.value;
            vid.recalc_refdef = true;
        }

        if (_scr.oldScrViewSize != scr_viewsize.value) {
            _scr.oldScrViewSize = scr_viewsize.value;
            vid.recalc_refdef = true;
        }

        if (vid.recalc_refdef)
            SCR_CalcRefdef();

        //
        // do 3D refresh drawing, and then update the screen
        //
        SCR_SetUpToDrawConsole();

        V_RenderView();

        GL_Set2D();

        //
        // draw any areas not covered by the refresh
        //
        SCR_TileClear();

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
        else {
            if (crosshair.value)
                Draw_Character(
                    scr_vrect.x + scr_vrect.width / 2 + cl_crossx.value,
                    scr_vrect.y + scr_vrect.height / 2 + cl_crossy.value,
                    '+'
                );

            SCR_DrawRam();
            SCR_DrawNet();
            SCR_DrawTurtle();
            SCR_DrawPause();
            SCR_CheckDrawCenterString();
            Sbar_Draw();
            SCR_DrawConsole();
            M_Draw();
        }

        V_UpdatePalette();

    } GL_EndRendering();
}

