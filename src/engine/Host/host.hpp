#pragma once

#include "host.h"

//
// host
//
class Host {
public:

    void ClearMemory();
    void Init(QuakeParms_p parms);
    void Shutdown();
    void  Error(cString error, ...);
    void  EndGame(cString message, ...);
    void Frame(float time);
    void Quit_f();
    void  ClientCommands(cString fmt, ...);
    void ShutdownServer(bool crash);
    bool IsDedicated();
    bool IsServerActive();

#if 0
    QuakeParms_t parms;

    bool     initialized;  // true if into command execution
    LegacyTimeStamp_t   frametime;

    uint8_p  basepal;
    uint8_p  colormap;
    int32_t  framecount; // incremented every frame, never reset
    LegacyTimeStamp_t   realtime;   // not bounded in any way, changed at start of every frame, never reset
#endif

  private:
    void InitCommands();
    void InitLocal();
    void FindMaxClients();
    bool FilterTime(float time);
    void GetConsoleCommands();
    void InitVCR(QuakeParms_p parms);
    void WriteConfiguration();
    void ServerFrame();
    void _Frame(float time);

};

extern Host host;
