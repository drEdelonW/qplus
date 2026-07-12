#pragma once

extern bool r_cache_thrash; // set if thrashing the surface cache. OpenGL compatability;

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