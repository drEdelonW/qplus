#pragma once

#include "Model_st.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* console/host commands */
    void Host_Quit_f();
    void Host_Status_f();
    void Host_God_f();
    void Host_Notarget_f();
    void Host_Noclip_f();
    void Host_Fly_f();
    void Host_Ping_f();

    void Host_Map_f();
    void Host_Changelevel_f();
    void Host_Restart_f();
    void Host_Reconnect_f();
    void Host_Connect_f();

    void Host_SavegameComment(cString text);   /* text: buffer size >= SAVEGAME_COMMENT_LENGTH+1 */
    void Host_Savegame_f();
    void Host_Loadgame_f();

#ifdef QUAKE2
    void SaveGamestate();
    int  LoadGamestate(cString level, cString startspot);
    void Host_Changelevel2_f();
#endif

    void Host_Name_f();
    void Host_Version_f();

#ifdef IDGODS
    void Host_Please_f();
#endif

    /* chat */
    void Host_Say(bool teamonly);
    void Host_Say_f();
    void Host_Say_Team_f();
    void Host_Tell_f();

    /* player/color */
    void Host_Color_f();
    void Host_Kill_f();
    void Host_Pause_f();

    /* spawn/signon */
    void Host_PreSpawn_f();
    void Host_Spawn_f();
    void Host_Begin_f();

    /* admin */
    void Host_Kick_f();
    void Host_Give_f();
    /* view helpers */
    void    Host_Viewmodel_f();
    void    Host_Viewframe_f();
    void    PrintFrameName(Model_p mdl, int frame);
    void    Host_Viewnext_f();
    void    Host_Viewprev_f();

    /* demo loop control */
    void Host_Startdemos_f();
    void Host_Demos_f();
    void Host_Stopdemo_f();

    /* register all commands */

#ifdef __cplusplus
} /* extern "C" */
#endif