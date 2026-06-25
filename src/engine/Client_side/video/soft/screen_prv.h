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
    LegacyTimeDelta_t    disabled_time;
    LegacyTimeDelta_t    centertime_start; // for slow victory printing
    int     center_lines;
    char    centerstring[1024];
    int      clearConsole;
    float    oldScrViewSize, oldFov;
} _Screen_t;

extern _Screen_t _scr;

#ifdef __cplusplus
extern "C" {
#endif

    void SCR_ScreenShot_f();
    void SCR_SizeUp_f();
    void SCR_SizeDown_f();
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

#ifdef __cplusplus
}
#endif