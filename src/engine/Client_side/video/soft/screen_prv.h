#pragma once

#include "qPic.h"
typedef struct {
    bool    initialized;  // ready to draw
    qPic_p  ram;
    qPic_p  net;
    qPic_p  turtle;
    bool    drawloading;
    bool    drawdialog;
    int     erase_lines;
    int     erase_center;
    cString notifystring;
    LegDt_t    disabled_time;
    LegDt_t    centertime_start; // for slow victory printing
    int     center_lines;
    char    centerstring[1024];
    int      clearConsole;
    float    oldViewSize, oldFov;
} _Screen_t;

extern _Screen_t _scr;
extern int fullupdate; // set to 0 to force full redraw

#ifdef __cplusplus
extern "C" {
#endif

    void SCR_Composite();
    void SCR_ScreenShot_f();
    void SCR_DrawCenterString();
    int SCR_ModalMessage(cString text);
    void SCR_DrawNotifyString();
    void SCR_BringDownConsole();
    void SCR_CheckDrawCenterString();
    float CalcFov(float fov_x, float width, float height);
    void SCR_SetUpToDrawConsole();
    void SCR_DrawConsole();

    void SCR_DrawRam();
    void SCR_DrawTurtle();
    void SCR_DrawNet();
    void SCR_DrawPause();
    void SCR_DrawLoading();
    void SCR_CalcRefdef();

    void Con_MessageMode_f();
    void Con_MessageMode2_f();
    void Con_DrawNotify();

#ifdef __cplusplus
}
#endif