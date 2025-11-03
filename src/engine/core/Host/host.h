#pragma once

#include "qparams.h"

//
// host
//
extern QuakeParms_t host_parms;

extern bool    host_initialized;  // true if into command execution
extern double  host_frametime;

extern uint8_p host_basepal;
extern uint8_p host_colormap;
extern int32_t host_framecount; // incremented every frame, never reset
extern double  realtime;   // not bounded in any way, changed at start of every frame, never reset
#ifdef __cplusplus
extern "C" {
#endif
    void Host_ClearMemory();
    void Host_ServerFrame();
    void Host_InitCommands();
    void Host_Init(QuakeParms_p parms);
    void Host_Shutdown();
    void Host_Error(cString error, ...);
    void Host_EndGame(cString message, ...);
    void Host_Frame(float time);
    void Host_Quit_f();
    void Host_ClientCommands(cString fmt, ...);
    void Host_ShutdownServer(bool crash);

    void Host_InitCommands(void);
#ifdef __cplusplus
}
#endif