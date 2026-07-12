#pragma once

#include "qPic.h"
extern bool r_cache_thrash; // set if thrashing the surface cache. OpenGL compatability;

typedef struct {
    qPic_p  ram;
    qPic_p  net;
    qPic_p  turtle;
    qPic_p  disc;
} Hid_t;
extern Hid_t hid;

static inline void HUD_Init() {
#if 1 /* System status */
    hid.ram = GetPicFromWad("ram");
    hid.net = GetPicFromWad("net");
    hid.turtle = GetPicFromWad("turtle");
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

    void SCR_DrawPause();
    void SCR_DrawLoading();
    void Con_DrawNotify();
    void Draw_BeginDisc();
    void Draw_EndDisc();

    void SCR_DrawRam();
    void SCR_DrawTurtle();
    void SCR_DrawNet();

#ifdef __cplusplus
}
#endif