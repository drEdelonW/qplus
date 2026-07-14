#pragma once

#include "qparams.h"
#include "qTime.h"
#include "qColor.h"
#include "qLight.h"

//
// host
//
extern bool    isDedicated;

extern QuakeParms_t host_parms;
extern bool    host_initialized;  // true if into command execution
extern SimDt_t host_frametime;
extern int32_t host_framecount; // incremented every frame, never reset
// extern jmp_buf host_abortserver;

#if 0
extern RealTime_t  realtime;   // not bounded in any way, changed at start of every frame, never reset
#else
    extern SimTime_t  _hosttime;    // was used in menu
    static inline SimTime_t GetHostTime() { return _hosttime; }
    static inline void AddHostTime(SimDt_t delta) { _hosttime += delta; }
    extern RealTime_t _realtime;
    static inline RealTime_t GetRealTime() { return _realtime; }
    extern ViewTime_t viewtime;
    static inline ViewTime_t GetVievTime() { return viewtime; }
#endif
#ifdef __cplusplus
extern "C" {
#endif

    void Host_ClearMemory();
    void Host_Init(QuakeParms_p parms);
    void Host_Shutdown();
    void Host_Printf(cStringRO fmt, ...);
    Q_NORETURN void Host_Error(cString error, ...);
    Q_NORETURN void Host_SysError(cStringRO error, ...);
    void Host_EndGame(cString message, ...);
    void Host_Frame(RealDt_t time);
    void Host_Quit_f();
    void Host_ClientCommands(cString fmt, ...);
    void Host_ShutdownServer(bool crash);
    LegTime_t Host_FloatTime();
    bool Host_IsDedicated();
    bool Host_IsServerActive();

#ifdef __cplusplus
}
#endif