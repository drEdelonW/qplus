#include "screen.h"
#include "screen_prv.h"
#include "cvar_q1.h"
#include <string.h>
#include "client.h"
#include "cmd.h"
#include "draw.h"
#include "host.h"
#include "sound.h"
#include "sys.h"
#include "console.h"
#include "sbar.h"
#include "RefDef.h"
#include <math.h>

Screen_t scr;
_Screen_t _scr;

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
#ifdef GLQUAKE
    Cvar_RegisterVariable(&gl_triplebuffer);
#endif
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

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
int SCR_ModalMessage(cString text) {
    if (Host_IsDedicated()) return true;

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
        (key.lastpress != K_ESCAPE)
        );

    scr.fullupdate = 0;
    SCR_UpdateScreen();

    return key.lastpress == 'y';
}



void SCR_DrawNotifyString() {
    cString start = _scr.notifystring;
    int y = vid.height * 0.35f;

    do {
        // scan the width of the line
        int inLine = 0;
        for (; inLine < 40; inLine++)
            if ((start[inLine] == '\n') ||
                (!start[inLine])
                )
                break;

        int x = (vid.width - inLine * 8) / 2;
        for (int j = 0; j < inLine; j++, x += 8)
            Draw_Character(x, y, start[j]);

        y += 8;

        while ((*start) && (*start != '\n'))
            start++;

        if (!*start)    break;
        start++;    // skip the \n
    } while (1);
}

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

    cl.cshifts[0].percent = 0;        // no area contents palette on next frame
    VID_SetPalette(host_basepal);
}



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

void SCR_CheckDrawCenterString() {
    scr.copytop = true;
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

/*
====================
CalcFov
====================
*/
float CalcFov(float fov_x, float width, float height) {
    if ((fov_x < 1) || (fov_x > 179))   Host_SysError("Bad fov: %f", fov_x);

    float x = width / tan(fov_x / 360 * M_PI);
    float at = atan(height / x) * 360 / M_PI;

    return at;
}



/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole() {
    Con_CheckResize();

    if (_scr.drawloading)   return; // never a console with loading plaque

    // decide on the height of the console
    con.forcedup = !cl.worldmodel || cls.signon != SIGNONS;

    if (con.forcedup) {
        scr.conlines = vid.height;  // full screen
        scr.con_current = scr.conlines;
    }
    else if (key.dest == key_console)   scr.conlines = vid.height / 2;  // half screen
    else                                scr.conlines = 0;               // none visible

    if (scr.con_current > scr.conlines) {
        scr.con_current -= scr_conspeed.value * host_frametime;
        if (scr.con_current < scr.conlines)
            scr.con_current = scr.conlines;

    }
    else if (scr.con_current < scr.conlines) {
        scr.con_current += scr_conspeed.value * host_frametime;
        if (scr.con_current > scr.conlines)
            scr.con_current = scr.conlines;
    }
    if (_scr.clearConsole++ < vid.numpages) {
#ifdef GLQUAKE
#else
        scr.copytop = true;
        Draw_TileClear(
            0,
            (int)scr.con_current,
            vid.width,
            vid.height - (int)scr.con_current
        );
#endif
        Sbar_Changed();
    }
    else if (scr.clearnotify++ < vid.numpages) {
#ifdef GLQUAKE
#else
        scr.copytop = true;
        Draw_TileClear(0, 0, vid.width, con.notifylines);
#endif
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
        scr.copyeverything = true;
        Con_DrawConsole(scr.con_current, true);
        _scr.clearConsole = 0;
    }
    else {
        if ((key.dest == key_game) ||
            (key.dest == key_message)
            )
            Con_DrawNotify();   // only draw notify in game
    }
}


#ifdef GLQUAKE
#else
#endif