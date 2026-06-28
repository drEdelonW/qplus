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
#include "menu.h"
#ifdef GLQUAKE
# include "qOpenGL.h"
#else
# include "render.h"
#endif

Screen_t scr;
_Screen_t _scr;

/*
==================
SCR_Init
==================
*/
void SCR_Init() {
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
    Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
    Cmd_AddCommand("screenshot", SCR_ScreenShot_f);
    Cmd_AddCommand("messagemode", Con_MessageMode_f);
    Cmd_AddCommand("messagemode2", Con_MessageMode2_f);


#if 1 /* System status */
    _scr.ram = Draw_PicFromWad("ram");
    _scr.net = Draw_PicFromWad("net");
    _scr.turtle = Draw_PicFromWad("turtle");
#endif

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
        vid.scr.height * 0.35 : 48;

    do {
        // scan the width of the line
        int inLine = 0;
        for (; inLine < 40; inLine++)
            if ((start[inLine] == '\n') || !start[inLine])
                break;

        int x = HALF(vid.scr.width - OCTO(inLine));
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
    SCR_RequestRedraw();
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

    SCR_RequestRedraw();
    SCR_UpdateScreen();

    return key.lastpress == 'y';
}



void SCR_DrawNotifyString() {
    cString start = _scr.notifystring;
    int y = vid.scr.height * 0.35f;

    do {
        // scan the width of the line
        int inLine = 0;
        for (; inLine < 40; inLine++)
            if ((start[inLine] == '\n') ||
                (!start[inLine])
                )
                break;

        int x = HALF(vid.scr.width - OCTO(inLine));
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
    SCR_RequestRedraw();
    Sbar_Changed();
    SCR_UpdateScreen();
    _scr.drawloading = false;

    scr.disabled_for_loading = true;
    _scr.disabled_time = realtime;
    SCR_RequestRedraw();
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque() {
    scr.disabled_for_loading = false;
    SCR_RequestRedraw();
    Con_ClearNotify();
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
    if (((realtime - cl.last_received_message) < 0.3) ||
        (cls.demoplayback))
        return;

    Draw_Pic(scr.vrect.x + 64, scr.vrect.y, _scr.net);
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

    /**/ if (con.forcedup) {
        scr.conlines = vid.scr.height;  // full screen
        scr.con_current = scr.conlines;
    }
    else if (key.dest == key_console)   scr.conlines = HALF(vid.scr.height);    // half screen
    else                                scr.conlines = 0;                       // none visible

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
            scr.con_current,
            vid.scr.width,
            vid.scr.height - scr.con_current
        );
#endif
        Sbar_Changed();
    }
    else if (scr.clearnotify++ < vid.numpages) {
#ifdef GLQUAKE
#else
        scr.copytop = true;
        Draw_TileClear(0, 0, vid.scr.width, con.notifylines);
#endif
    }
    else
        con.notifylines = 0;
}

bool recalc_refdef;  // if true, recalc vid-based stuff
void SCR_RequestCalcRefdef() {
    recalc_refdef = true;
}
/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
void SCR_CalcRefdef() {
    if (!recalc_refdef) return;
    else recalc_refdef = false;

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
    {
        float size = (cl.intermission != IM_NONE) ? 120 : scr_viewsize.value;
        /**/ if (size >= 120.0f)    sb_lines = 0;        // no status bar at all
        else if (size >= 110.0f)    sb_lines = 24;       // no inventory
        else /*               */    sb_lines = 24 + 16 + 8;
    }

    vRect_t vrect = {
        .width = vid.scr.width,
        .height = vid.scr.height
    };
    vRect_p pvrectin = &vrect;
    vRect_p pvrect = &r_refdef.vrect;
    int lineadj = sb_lines;
#ifdef GLQUAKE
    bool full = ((scr_viewsize.value >= 100.0f) || (cl.intermission != IM_NONE));
    /* look like void R_SetVrect(vRect_p pvrectin, vRect_p pvrect, int lineadj) in r_main.c */ {
        float size = (scr_viewsize.value > 100.0f) ?
            100.0f : scr_viewsize.value;

        if (cl.intermission != IM_NONE) {
            size = 100.0f;
            lineadj = 0;
        }
        size /= 100.0f;

        int h = pvrectin->height - lineadj;
        pvrect->width = pvrectin->width * size;
        if (pvrect->width < 96.0f) {
            size = 96.0f / pvrect->width;
            pvrect->width = 96.0f;    // min for icons
        }

        pvrect->height = pvrectin->height * size;
        if (pvrect->height > (pvrectin->height - lineadj))
            pvrect->height = (pvrectin->height - lineadj);

        {   /* GLQUAKE specific */
            if (pvrect->height > pvrectin->height)
                pvrect->height = pvrectin->height;

            pvrect->x = (pvrectin->width - pvrect->width) / 2;
            pvrect->y = (full) ? 0 : (h - pvrect->height) / 2;
        }
    }
#else
    // these calculations mirror those in R_Init() for r_refdef, but take no account of water warping

    R_SetVrect(pvrectin, &scr.vrect, lineadj);
#endif

    r_refdef.fov_x = scr_fov.value;
    r_refdef.fov_y = CalcFov(r_refdef.fov_x, pvrect->width, pvrect->height);

#ifdef GLQUAKE
#else
    // guard against going from one mode to another that's less than half the vertical resolution
    if (scr.con_current > vid.scr.height)
        scr.con_current = vid.scr.height;

    // notify the refresh of the change
    R_ViewChanged(pvrectin, sb_lines, scr.aspect);
#endif
}




int fullupdate; // set to 0 to force full redraw
void SCR_RequestRedraw() {
    fullupdate = 0;
}

// scr_common.c

/*
==================
SCR_Composite
Composite HUD, intermission, dialog and loading
overlays based on current game state.
Called from platform-specific SCR_UpdateScreen.
==================
*/
void SCR_Composite() {
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
}