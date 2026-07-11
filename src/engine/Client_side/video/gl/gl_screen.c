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
#include "view.h"
#include "cvar_q1.h"
#include <stdlib.h>

#include "screen_prv.h"

int glx, gly, glwidth, glheight; // extern
cvar_t  gl_triplebuffer = { "gl_triplebuffer", "1", true };

bool    scr_disabled_for_loading;

/*
==============================================================================

                        SCREEN SHOTS

==============================================================================
*/

#include "tga.h"

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
    // -- pcxname[5/6] is ^^ this positions
    int i = 0;
    for (; i <= 99; i++) {
        pcxname[5] = i / 10 + '0';
        pcxname[6] = i % 10 + '0';
        fsPath_t checkname;
        snprintf(checkname, sizeof(checkname), "%s/%s", com.gamedir, pcxname);
        if (Sys_FileTime(checkname) == -1)
            break;    // file doesn't exist
    }
    if (i == 100) {
        Con_Printf("SCR_ScreenShot_f: Couldn't create a PCX file\n");
        return;
    }

#if 0
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
#else
    int buffSz = glwidth * glheight * 3;
    uint8_p frBuff = malloc(buffSz);
    glReadPixels(
        glx, gly,
        glwidth, glheight,
        GL_RGB, GL_UNSIGNED_BYTE,
        frBuff
    );

    WriteTGAfile(
        pcxname, frBuff,
        glwidth, glheight,
        0, NULL
    );
    free(frBuff);

#endif
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
            Scr.vrect.height - sb_lines
        );
        // right
        Draw_TileClear(
            r_refdef.vrect.x + r_refdef.vrect.width,
            0,
            Scr.vrect.width - r_refdef.vrect.x + r_refdef.vrect.width,
            Scr.vrect.height - sb_lines
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
            Scr.vrect.height - sb_lines - (r_refdef.vrect.height + r_refdef.vrect.y)
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
    if (Scr.block_drawing)  return;

    // Scr.numpages = 2 + gl_triplebuffer.value;

#if 0   /* this specific for software render. not applicable for OpenGL */
    scr.copytop = false;        // TODO: wrap this valuse to avoid global publishing
    scr.copyeverything = false; // TODO: wrap this valuse to avoid global publishing
#endif

    if (scr_disabled_for_loading) {
        if ((GetRealTime() - _scr.disabled_time) > 60) {
            scr_disabled_for_loading = false;
            Con_Printf("load failed.\n");
        }
        else    return;
    }

    if (Host_IsDedicated())     return; // stdout only

    if (!_scr.initialized || !con.isInitialized)
        return;     // not initialized yet


    GL_BeginRendering(&glx, &gly, &glwidth, &glheight); {

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
            SCR_Composite();    // main state based compositor
        V_UpdatePalette();

    } GL_EndRendering();
}

