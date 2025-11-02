#pragma once

#include "host.h"
#include "qparams.h"

//
// host
//
class Host {
public:
    // Host();

    void  ClearMemory();
    void  ServerFrame();
    void  InitCommands();
    void  Init(QuakeParms_p parms);
    void  Shutdown();
    void  Error(cString error, ...);
    void  EndGame(cString message, ...);
    void  Frame(float time);
    void  Quit_f();
    void  ClientCommands(cString fmt, ...);
    void  ShutdownServer(bool crash);

    QuakeParms_t parms;

    bool     initialized;  // true if into command execution
    double   frametime;

    uint8_p  basepal;
    uint8_p  colormap;
    int32_t  framecount; // incremented every frame, never reset
    double   realtime;   // not bounded in any way, changed at
    // start of every frame, never reset
    void InitLocal();
    void FindMaxClients();
    bool FilterTime(float time);
    void GetConsoleCommands();
    void _Frame(float time);
    void InitVCR(QuakeParms_p parms);
    void WriteConfiguration();
private:

};

// #ifndef HOST_CMDS_H
// #define HOST_CMDS_H
#include "progs.h"
#include "Model.h"
// #ifdef __cplusplus
// extern "C" {
// #endif

    /* extern globals */
    extern int32_t current_skill;
    extern bool    noclip_anglehack;

    /* from other TUs but needed here */
    void Mod_Print(void);
    void M_Menu_Quit_f(void);

    /* console/host commands */
    void Host_Quit_f(void);
    void Host_Status_f(void);
    void Host_God_f(void);
    void Host_Notarget_f(void);
    void Host_Noclip_f(void);
    void Host_Fly_f(void);
    void Host_Ping_f(void);

    void Host_Map_f(void);
    void Host_Changelevel_f(void);
    void Host_Restart_f(void);
    void Host_Reconnect_f(void);
    void Host_Connect_f(void);

    void Host_SavegameComment(cString text);   /* text: buffer size >= SAVEGAME_COMMENT_LENGTH+1 */
    void Host_Savegame_f(void);
    void Host_Loadgame_f(void);

#ifdef QUAKE2
    void SaveGamestate(void);
    int  LoadGamestate(cString level, cString startspot);
    void Host_Changelevel2_f(void);
#endif

    void Host_Name_f(void);
    void Host_Version_f(void);

#ifdef IDGODS
    void Host_Please_f(void);
#endif

    /* chat */
    void Host_Say(bool teamonly);
    void Host_Say_f(void);
    void Host_Say_Team_f(void);
    void Host_Tell_f(void);

    /* player/color */
    void Host_Color_f(void);
    void Host_Kill_f(void);
    void Host_Pause_f(void);

    /* spawn/signon */
    void Host_PreSpawn_f(void);
    void Host_Spawn_f(void);
    void Host_Begin_f(void);

    /* admin */
    void Host_Kick_f(void);

    /* view helpers */
    edict_p FindViewthing(void);
    void    Host_Viewmodel_f(void);
    void    Host_Viewframe_f(void);
    void    PrintFrameName(Model_p mdl, int frame);
    void    Host_Viewnext_f(void);
    void    Host_Viewprev_f(void);

    /* demo loop control */
    void Host_Startdemos_f(void);
    void Host_Demos_f(void);
    void Host_Stopdemo_f(void);

    /* register all commands */

// #ifdef __cplusplus
// } /* extern "C" */
// #endif

// #endif /* HOST_CMDS_H */
