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
#include "qPic.h"
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

Screen_t scr;
int            glx, gly, glwidth, glheight;

// only the refresh window will be updated unless these variables are flagged 
int     scr_copytop;
int     scr_copyeverything;

float   scr_con_current;
float   scr_conlines;        // lines of console to display

float   oldscreensize, oldfov;
cvar_t  gl_triplebuffer = { "gl_triplebuffer", "1", true };


bool    scr_initialized;        // ready to draw

qPic_p scr_ram;
qPic_p scr_net;
qPic_p scr_turtle;

int     scr_fullupdate;

int     clearconsole;
int     clearnotify;

int     sb_lines;

VidDef_t    vid;                // global video state

vRect_t     scr_vrect;

bool    scr_disabled_for_loading;
bool    scr_drawloading;
float   scr_disabled_time;

bool    block_drawing;

void SCR_ScreenShot_f();

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char    scr_centerstring[1024];
float   scr_centertime_start;    // for slow victory printing
float   scr_centertime_off;
int     scr_center_lines;
int     scr_erase_lines;
int     scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint(cString str) {
    strncpy(scr_centerstring, str, sizeof(scr_centerstring) - 1);
    scr_centertime_off = scr_centertime.value;
    scr_centertime_start = cl.time;

    // count the number of lines for centering
    scr_center_lines = 1;
    while (*str) {
        if (*str == '\n')
            scr_center_lines++;
        str++;
    }
}


void SCR_DrawCenterString() {

    int remaining;
    // the finale prints the characters one at a time
    if (cl.intermission != IM_NONE)     remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
    else                                remaining = 9999;

    scr_erase_center = 0;
    cString start = scr_centerstring;

    int y;
    if (scr_center_lines <= 4)      y = vid.height * 0.35;
    else                            y = 48;

    do {
        // scan the width of the line
        int l = 0;
        for (; l < 40; l++)
            if (start[l] == '\n' || !start[l])
                break;
        int x = (vid.width - l * 8) / 2;
        for (int j = 0; j < l; j++, x += 8) {
            Draw_Character(x, y, start[j]);
            if (!remaining--)
                return;
        }

        y += 8;

        while (*start && *start != '\n')
            start++;

        if (!*start)
            break;
        start++;        // skip the \n
    } while (1);
}

void SCR_CheckDrawCenterString() {
    scr_copytop = 1;
    if (scr_center_lines > scr_erase_lines)
        scr_erase_lines = scr_center_lines;

    scr_centertime_off -= host_frametime;

    if ((scr_centertime_off <= 0) && (cl.intermission == IM_NONE))
        return;
    if (key.dest != key_game)
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
    if ((fov_x < 1) || (fov_x > 179))   Host_SysError("Bad fov: %f", fov_x);

    float x = width / tan(fov_x / 360 * M_PI);

    float a = atan(height / x);

    a = a * 360 / M_PI;

    return a;
}

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

    if (size >= 120)            sb_lines = 0;        // no status bar at all
    else if (size >= 110)       sb_lines = 24;        // no inventory
    else                        sb_lines = 24 + 16 + 8;

    bool    full = false;
    if (scr_viewsize.value >= 100.0) {
        full = true;
        size = 100.0;
    }
    else
        size = scr_viewsize.value;
    if (cl.intermission != IM_NONE) {
        full = true;
        size = 100;
        sb_lines = 0;
    }
    size /= 100.0;

    int h = vid.height - sb_lines;

    r_refdef.vrect.width = vid.width * size;
    if (r_refdef.vrect.width < 96) {
        size = 96.0 / r_refdef.vrect.width;
        r_refdef.vrect.width = 96;    // min for icons
    }

    r_refdef.vrect.height = vid.height * size;
    if (r_refdef.vrect.height > vid.height - sb_lines)
        r_refdef.vrect.height = vid.height - sb_lines;
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
    Cvar_RegisterVariable(&gl_triplebuffer);

    //
    // register our commands
    //
    Cmd_AddCommand("screenshot", SCR_ScreenShot_f);
    Cmd_AddCommand("sizeup", SCR_SizeUp_f);
    Cmd_AddCommand("sizedown", SCR_SizeDown_f);

    scr_ram = Draw_PicFromWad("ram");
    scr_net = Draw_PicFromWad("net");
    scr_turtle = Draw_PicFromWad("turtle");

    scr_initialized = true;
}



/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam() {
    if (!scr_showram.value)     return;
    if (!r_cache_thrash)        return;

    Draw_Pic(scr_vrect.x + 32, scr_vrect.y, scr_ram);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle() {
    static int    _count;

    if (!scr_showturtle.value)  return;

    if (host_frametime < 0.1) {
        _count = 0;
        return;
    }

    _count++;
    if (_count < 3)
        return;

    Draw_Pic(scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet() {
    if ((realtime - cl.last_received_message) < 0.3)    return;
    if (cls.demoplayback)                               return;

    Draw_Pic(scr_vrect.x + 64, scr_vrect.y, scr_net);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause() {
    if (!scr_showpause.value)   return; // turn off for screenshots
    if (!cl.paused)             return;

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
    if (!scr_drawloading)
        return;

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

    if (scr_drawloading)
        return;        // never a console with loading plaque

    // decide on the height of the console
    con.forcedup = !cl.worldmodel || cls.signon != SIGNONS;

    if (con.forcedup) {
        scr_conlines = vid.height;        // full screen
        scr_con_current = scr_conlines;
    }
    else if (key.dest == key_console)
        scr_conlines = vid.height / 2;    // half screen
    else
        scr_conlines = 0;                // none visible

    if (scr_conlines < scr_con_current) {
        scr_con_current -= scr_conspeed.value * host_frametime;
        if (scr_conlines > scr_con_current)
            scr_con_current = scr_conlines;

    }
    else if (scr_conlines > scr_con_current) {
        scr_con_current += scr_conspeed.value * host_frametime;
        if (scr_conlines < scr_con_current)
            scr_con_current = scr_conlines;
    }

    if (clearconsole++ < vid.numpages) {
        Sbar_Changed();
    }
    else if (clearnotify++ < vid.numpages) {
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
    if (scr_con_current) {
        scr_copyeverything = 1;
        Con_DrawConsole(scr_con_current, true);
        clearconsole = 0;
    }
    else {
        if (key.dest == key_game || key.dest == key_message)
            Con_DrawNotify();    // only draw notify in game
    }
}


/*
==============================================================================

                        SCREEN SHOTS

==============================================================================
*/

typedef struct _TargaHeader {
    uint8_t     id_length, colormap_type, image_type;
    uint16_t    colormap_index, colormap_length;
    uint8_t    colormap_size;
    uint16_t    x_origin, y_origin, width, height;
    uint8_t    pixel_size, attributes;
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

    glReadPixels(glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, buffer + 18);

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


/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque() {
    S_StopAllSounds(true);

    if (cls.state != ca_connected)  return;
    if (cls.signon != SIGNONS)      return;

    // redraw with no console and the loading plaque
    Con_ClearNotify();
    scr_centertime_off = 0;
    scr_con_current = 0;

    scr_drawloading = true;
    scr_fullupdate = 0;
    Sbar_Changed();
    SCR_UpdateScreen();
    scr_drawloading = false;

    scr_disabled_for_loading = true;
    scr_disabled_time = realtime;
    scr_fullupdate = 0;
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque() {
    scr_disabled_for_loading = false;
    scr_fullupdate = 0;
    Con_ClearNotify();
}

//=============================================================================

cString scr_notifystring;
bool    scr_drawdialog;

void SCR_DrawNotifyString() {
    cString start = scr_notifystring;
    int y = vid.height * 0.35;

    do {
        // scan the width of the line
        int l = 0;
        for (; l < 40; l++)
            if ((start[l] == '\n') ||
                (!start[l])
                )
                break;
        int x = (vid.width - l * 8) / 2;
        for (int j = 0; j < l; j++, x += 8)
            Draw_Character(x, y, start[j]);

        y += 8;

        while ((*start) && (*start != '\n'))
            start++;

        if (!*start)
            break;
        start++;        // skip the \n
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
    if (Host_IsDedicated())
        return true;

    scr_notifystring = text;

    // draw a fresh screen
    scr_fullupdate = 0;
    scr_drawdialog = true;
    SCR_UpdateScreen();
    scr_drawdialog = false;

    S_ClearBuffer();        // so dma doesn't loop current sound

    do {
        key.count = -1;        // wait for a key down and up
        Sys_SendKeyEvents();
    } while (
        (key.lastpress != 'y') &&
        (key.lastpress != 'n') &&
        (key.lastpress != K_ESCAPE)
        );

    scr_fullupdate = 0;
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
    scr_centertime_off = 0;

    for (int i = 0; i < 20 && scr_conlines != scr_con_current; i++)
        SCR_UpdateScreen();

    cl.cshifts[0].percent = 0;        // no area contents palette on next frame
    VID_SetPalette(host_basepal);
}

void SCR_TileClear() {
    if (r_refdef.vrect.x > 0) {
        // left
        Draw_TileClear(0, 0, r_refdef.vrect.x, vid.height - sb_lines);
        // right
        Draw_TileClear(r_refdef.vrect.x + r_refdef.vrect.width, 0,
            vid.width - r_refdef.vrect.x + r_refdef.vrect.width,
            vid.height - sb_lines);
    }
    if (r_refdef.vrect.y > 0) {
        // top
        Draw_TileClear(r_refdef.vrect.x, 0,
            r_refdef.vrect.x + r_refdef.vrect.width,
            r_refdef.vrect.y);
        // bottom
        Draw_TileClear(r_refdef.vrect.x,
            r_refdef.vrect.y + r_refdef.vrect.height,
            r_refdef.vrect.width,
            vid.height - sb_lines -
            (r_refdef.vrect.height + r_refdef.vrect.y));
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
    if (block_drawing)
        return;

    vid.numpages = 2 + gl_triplebuffer.value;

    scr_copytop = 0;
    scr_copyeverything = 0;

    if (scr_disabled_for_loading) {
        if (realtime - scr_disabled_time > 60) {
            scr_disabled_for_loading = false;
            Con_Printf("load failed.\n");
        }
        else    return;
    }

    if (!scr_initialized || !con.isInitialized)
        return;                // not initialized yet


    GL_BeginRendering(&glx, &gly, &glwidth, &glheight);

    //
    // determine size of refresh window
    //
    if (oldfov != scr_fov.value) {
        oldfov = scr_fov.value;
        vid.recalc_refdef = true;
    }

    if (oldscreensize != scr_viewsize.value) {
        oldscreensize = scr_viewsize.value;
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

    if (scr_drawdialog) {
        Sbar_Draw();
        Draw_FadeScreen();
        SCR_DrawNotifyString();
        scr_copyeverything = true;
    }
    else if (scr_drawloading) {
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
            Draw_Character(scr_vrect.x + scr_vrect.width / 2, scr_vrect.y + scr_vrect.height / 2, '+');

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

    GL_EndRendering();
}

