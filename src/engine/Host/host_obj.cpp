/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// host.c -- coordinates spawning and killing of local servers

#include "host.hpp"
#include "host_cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "d_iface.h"
#include "server.h"
#undef SERVER   // TODO: remove this workaround
#include "common.h"
#include "sys.h"
#include "protocol.h"
#include "keys.h"
#include "console.h"
#include "vid.h"
#include "cmd.h"
#include "cbuf.h"
#include "input.h"
#include "sound.h"
#include "cdaudio.h"
#include "wad.h"
#include "view.h"
#include "draw.h"
#include "menu.h"
#include "sbar.h"
#include "chase.h"
#ifndef GLQUAKE
#   include "r_local.h"
#else
#   include "qOpenGL.h"
#   include "client.h"
#   include "cvar_q1.h"
#   include "common.h"
#endif
#include "mathlib.h"
#include "screen.h"
#include "q_tools.h"
#include "msg.h"
#include "net_vcr.h"
#include <setjmp.h>
#include "progs.h"
#include "gamedefs.h"
#include "GlobVars.h"


/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

#if 1
QuakeParms_t host_parms;
bool    host_initialized;   // true if into command execution
double  host_frametime;
double  host_time;
int32_t host_framecount;
int     host_hunklevel;
jmp_buf host_abortserver;
uint8_p host_basepal;
uint8_p host_colormap;
bool    isDedicated;
double  realtime;           // without any filtering or bounding
double  oldrealtime;        // last frame run
size_t  minimum_memory;
#endif



/*
================
Host::EndGame
================
*/
void Host::EndGame(cString message, ...) {
    va_list  argptr;    va_start(argptr, message);
    char string[1024]; vsnprintf(string, sizeof(string), message, argptr);
    va_end(argptr);
    Con_DPrintf("Host::EndGame: %s\n", string);

    if (SV_IsActive())  ShutdownServer(false);

    if (Host_IsDedicated())  Host_SysError("Host::EndGame: %s\n", string); // dedicated servers exit

    if (cls.demonum != -1)  CL_NextDemo();
    else                    CL_Disconnect();

    longjmp(host_abortserver, 1);
}

/*
================
Host::Error

This shuts down both the client and server
================
*/
void Host::Error(cString error, ...) {
    static bool _inError = false;

    if (_inError)    Host_SysError("Host::Error: recursively entered");
    _inError = true;

    SCR_EndLoadingPlaque();  // reenable screen updates

    va_list  argptr;    va_start(argptr, error);
    char  string[1024]; vsnprintf(string, sizeof(string), error, argptr);
    va_end(argptr);
    Con_Printf("Host::Error: %s\n", string);

    if (SV_IsActive())  ShutdownServer(false);

    if (Host_IsDedicated())  Host_SysError("Host::Error: %s\n", string); // dedicated servers exit

    CL_Disconnect();
    cls.demonum = -1;

    _inError = false;

    longjmp(host_abortserver, 1);
}

/*
================
Host::FindMaxClients
================
*/
void Host::FindMaxClients() {
    svs.maxClients = 1;

    int param = COM_CheckParm("-dedicated");
    if (param) {
        cls.state = ca_dedicated;
        if (param != (com.argc - 1))    svs.maxClients = Q_atoi(com.argv[param + 1]);
        else                            svs.maxClients = 8;
    }
    else        cls.state = ca_disconnected;

    param = COM_CheckParm("-listen");
    if (param) {
        if (Host_IsDedicated())  Host_SysError("Only one of -dedicated or -listen can be specified");

        if (param != (com.argc - 1))    svs.maxClients = Q_atoi(com.argv[param + 1]);
        else                            svs.maxClients = 8;
    }
    if (svs.maxClients < 1)                     svs.maxClients = 8;
    else if (svs.maxClients > MAX_SCOREBOARD)   svs.maxClients = MAX_SCOREBOARD;

    svs.maxClientsLimit = svs.maxClients;
    if (svs.maxClientsLimit < MAX_CLIENT_LIMIT) svs.maxClientsLimit = MAX_CLIENT_LIMIT;
    svs.clients = (RmtClient_p)Hunk_AllocName(svs.maxClientsLimit * sizeof(RmtClient_t), "clients");

    if (svs.maxClients > 1) Cvar_SetValue("deathmatch", 1.0);
    else                    Cvar_SetValue("deathmatch", 0.0);
}


/*
=======================
Host::Host
======================
*/
// Host::Host() {
void Host::InitLocal() {
    InitCommands();

    Cvar_RegisterVariable(&host_framerate);
    Cvar_RegisterVariable(&host_speeds);

    Cvar_RegisterVariable(&sys_ticrate);
    Cvar_RegisterVariable(&serverprofile);

    Cvar_RegisterVariable(&fraglimit);
    Cvar_RegisterVariable(&timelimit);
    Cvar_RegisterVariable(&teamplay);
    Cvar_RegisterVariable(&samelevel);
    Cvar_RegisterVariable(&noexit);
    Cvar_RegisterVariable(&skill);
    Cvar_RegisterVariable(&developer);
    Cvar_RegisterVariable(&deathmatch);
    Cvar_RegisterVariable(&coop);
    Cvar_RegisterVariable(&pausable);
    Cvar_RegisterVariable(&temp1);

    FindMaxClients();

    host_time = 1.0;  // so a think at time 0 won't get called
}


/*
===============
Host::WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host::WriteConfiguration() {
    // dedicated servers initialize the host but don't parse and set the
    // config.cfg cvars
    if (host_initialized & !isDedicated) {
        FILE* f = fopen(va("%s/config.cfg", com.gamedir), "w");
        if (!f) {
            Con_Printf("Couldn't write config.cfg.\n");
            return;
        }

        Key_WriteBindings(f);
        Cvar_WriteVariables(f);

        fclose(f);
    }
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
// void SV_ClientPrintf(cString fmt, ...) {
//     va_list  argptr;    va_start(argptr, fmt);
//     char  string[1024]; vsnprintf(string, sizeof(string), fmt, argptr);
//     va_end(argptr);
//     sizebuf_p pBuf = &remoteClient->message;
//     MSG_WriteByte(pBuf, svc_print); MSG_WriteString(pBuf, string);
// }

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
// void SV_BroadcastPrintf(cString fmt, ...) {
//    va_list  argptr;    va_start(argptr, fmt);
//    char  string[1024]; vsnprintf(string, sizeof(string), fmt, argptr);
//    va_end(argptr);
//     for (int i = 0; i < svs.maxClients; i++)
//         if (svs.clients[i].active && svs.clients[i].spawned) {
    //     sizebuf_p pBuf = &svs.clients[i].message;
//             MSG_WriteByte(pBuf, svc_print);   MSG_WriteString(pBuf, string);
//         }
// }

/*
=================
Host::ClientCommands

Send text over to the client to be executed
=================
*/
void Host::ClientCommands(cString fmt, ...) {
    va_list  argptr;    va_start(argptr, fmt);
    char  string[1024]; vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);

    sizebuf_p pBuf = &remoteClient->message;
    MSG_WriteByte(pBuf, svc_stufftext); MSG_WriteString(pBuf, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient(bool crash) {
    if (!crash) {
        // send any final messages (don't check for errors)
        if (NET_CanSendMessage(remoteClient->netconnection)) {
            sizebuf_p pBuf = &remoteClient->message;
            MSG_WriteByte(pBuf, svc_disconnect);    NET_SendMessage(remoteClient->netconnection, pBuf);
        }

        if (remoteClient->edict && remoteClient->spawned) {
            // call the prog function for removing a client
            // this will set the body to a dead frame, among other things
            int saveSelf = pr_global_struct->self;
            pr_global_struct->self = ED_GetEDictOffs(remoteClient->edict);
            PR_ExecuteProgram(pr_global_struct->ClientDisconnect);
            pr_global_struct->self = saveSelf;
        }

        Host_Printf("Client %s removed\n", remoteClient->name);
    }

    // break the net connection
    NET_Close(remoteClient->netconnection);
    remoteClient->netconnection = NULL;

    // free the client (the body stays around)
    remoteClient->active = false;
    remoteClient->name[0] = 0;
    remoteClient->old_frags = -16959;//-999999;
    net_activeconnections--;

    // send notification to all clients
    RmtClient_p client = svs.clients;
    for (int i = 0; i < svs.maxClients; i++, client++) {
        if (!client->active)    continue;
        sizebuf_p pBuf = &client->message;
        MSG_WriteByte(pBuf, svc_updatename);    MSG_WriteByte(pBuf, (uint8_t)(remoteClient - svs.clients));    MSG_WriteString(pBuf, "");
        MSG_WriteByte(pBuf, svc_updatefrags);   MSG_WriteByte(pBuf, (uint8_t)(remoteClient - svs.clients));    MSG_WriteShort(pBuf, 0);
        MSG_WriteByte(pBuf, svc_updatecolors);  MSG_WriteByte(pBuf, (uint8_t)(remoteClient - svs.clients));    MSG_WriteByte(pBuf, 0);
    }
}

/*
==================
Host::ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
// TDDO: move this logic to Server private
void Host::ShutdownServer(bool crash) {
    if (!SV_IsActive()) return;

    sv.active = false;

    // stop all client sounds immediately
    if (cls.state == ca_connected)  CL_Disconnect();

    // flush any pending messages - like the score!!!
    double start = Host_FloatTime();
    int  count;
    do {
        count = 0;
        remoteClient = svs.clients;
        for (int i = 0; i < svs.maxClients; i++, remoteClient++)
            if (remoteClient->active && remoteClient->message.cursize) {
                if (NET_CanSendMessage(remoteClient->netconnection)) {
                    NET_SendMessage(remoteClient->netconnection, &remoteClient->message);
                    SZ_Clear(&remoteClient->message);
                }
                else {
                    NET_GetMessage(remoteClient->netconnection);
                    count++;
                }
            }
        if ((Host_FloatTime() - start) > 3.0)    break;
    } while (count);

    // make sure all the clients know we're disconnecting
    uint8_t  message[4];
    sizebuf_t buf = {
        .data = message,
        .maxsize = 4,
        .cursize = 0
    };
    MSG_WriteByte(&buf, svc_disconnect);
    {
        int32_t count = NET_SendToAll(&buf, 5);
        if (count)
            Con_Printf("Host::ShutdownServer: NET_SendToAll failed for %u clients\n", count);
    }
    remoteClient = svs.clients;
    for (int i = 0; i < svs.maxClients; i++, remoteClient++)
        if (remoteClient->active)
            SV_DropClient(crash);

    //
    // clear structures
    //
    memset(&sv, 0, sizeof(sv));
    memset(svs.clients, 0, svs.maxClientsLimit * sizeof(RmtClient_t));
}


/*
================
Host::ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host::ClearMemory() {
    Con_DPrintf("Clearing memory\n");
    D_FlushCaches();
    Mod_ClearAll();
    if (host_hunklevel)
        Hunk_FreeToLowMark(host_hunklevel);

    cls.signon = 0;
    memset(&sv, 0, sizeof(sv));
    memset(&cl, 0, sizeof(cl));
}


//============================================================================


/*
===================
Host::FilterTime

Returns false if the time is too int16_t to run a frame
===================
*/
bool Host::FilterTime(float time) {
    realtime += time;

    if (!cls.timedemo && ((realtime - oldrealtime) < (1.0 / 72.0)))
        return false;  // framerate is too high

    host_frametime = realtime - oldrealtime;
    oldrealtime = realtime;

    if (host_framerate.value > 0)
        host_frametime = host_framerate.value;
    else { // don't allow really long or int16_t frames
        if (host_frametime > 0.1)   host_frametime = 0.1;
        if (host_frametime < 0.001) host_frametime = 0.001;
    }

    return true;
}


/*
===================
Host::GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host::GetConsoleCommands() {
    while (1) {
        cString cmd = Sys_ConsoleInput();
        if (!cmd)
            break;
        Cbuf_AddText(cmd);
    }
}


/*
==================
Host::ServerFrame

==================
*/
#ifdef FPS_20

void Host::_ServerFrame() {
    // run the world state
    pr_global_struct->frametime = host_frametime;

    // read client messages
    SV_RunClients();

    // move things around and think
    // always pause in single player if in console or menus
    if (!sv.paused && ((svs.maxClients > 1) || (key.dest == key_game)))
        SV_Physics();
}

void Host::ServerFrame() {
    // run the world state
    pr_global_struct->frametime = host_frametime;

    // set the time and clear the general datagram
    SV_ClearDatagram();

    // check for new clients
    SV_CheckForNewClients();

    float save_host_frametime;
    float temp_host_frametime = save_host_frametime = host_frametime;
    while (temp_host_frametime > (1.0 / 72.0)) {
        if (temp_host_frametime > 0.05)     host_frametime = 0.05;
        else                                host_frametime = temp_host_frametime;
        temp_host_frametime -= host_frametime;
        _ServerFrame();
    }
    host_frametime = save_host_frametime;

    // send all messages to the clients
    SV_SendClientMessages();
}

#else

void Host::ServerFrame() {
    pr_global_struct->frametime = host_frametime;   // run the world state

    SV_ClearDatagram();     // set the time and clear the general datagram
    SV_CheckForNewClients();    // check for new clients
    SV_RunClients();    // read client messages

    // move things around and think
    // always pause in single player if in console or menus
    if (!sv.paused &&
        (
            (svs.maxClients > 1) ||
            (key.dest == key_game)
            )
        )
        SV_Physics();

    SV_SendClientMessages(); // send all messages to the clients
}

#endif


/*
==================
Host::Frame

Runs all active servers
==================
*/
void Host::_Frame(float time) {
    static double  _time1 = 0.0;
    static double  _time2 = 0.0;
    static double  _time3 = 0.0;

    if (setjmp(host_abortserver))
        return;   // something bad happened, or the server disconnected

    rand(); // keep the random time dependent

    // decide the simulation time
    if (!FilterTime(time)) return; // don't run too fast, or packets will flood out

    Sys_SendKeyEvents();    // get new key events
    IN_Commands();  // allow mice or other external controllers to add commands
    Cbuf_Execute(); // process console commands
    NET_Poll();

    // if running the server locally, make intentions now
    if (SV_IsActive())  CL_SendCmd();

    //-------------------
    //
    // server operations
    //
    //-------------------

    // check for commands typed to the host
    GetConsoleCommands();

    if (SV_IsActive())  ServerFrame();

    //-------------------
    //
    // client operations
    //
    //-------------------

    // if running the server remotely, send intentions now after
    // the incoming messages have been read
    if (!SV_IsActive()) CL_SendCmd();

    host_time += host_frametime;

    // fetch results from server
    if (cls.state == ca_connected)  CL_ReadFromServer();

    // update video
    if (host_speeds.value)  _time1 = Host_FloatTime();

    SCR_UpdateScreen();

    if (host_speeds.value)  _time2 = Host_FloatTime();

    // update audio
    if (cls.signon == SIGNONS) {
        S_Update(r_origin, vpn, vright, vup);
        CL_DecayLights();
    }
    else
        S_Update(vec3_origin, vec3_origin, vec3_origin, vec3_origin);

    CDAudio_Update();

    if (host_speeds.value) {
        int pass1 = (_time1 - _time3) * 1000;
        _time3 = Host_FloatTime();
        int pass2 = (_time2 - _time1) * 1000;
        int pass3 = (_time3 - _time2) * 1000;
        Con_Printf("%3i tot %3i server %3i gfx %3i snd\n",
            pass1 + pass2 + pass3, pass1, pass2, pass3);
    }

    host_framecount++;
}

void Host::Frame(float time) {
    static double   _timeTotal;
    static int      _timeCount = 0;

    if (!serverprofile.value) { _Frame(time); return; }

    double time1 = Host_FloatTime();
    _Frame(time);
    double time2 = Host_FloatTime();

    _timeTotal += time2 - time1;
    _timeCount++;

    if (_timeCount < 1000)
        return;

    int m = _timeTotal * 1000 / _timeCount;
    _timeCount = 0;
    _timeTotal = 0;
    int numCli = 0;
    for (int i = 0; i < svs.maxClients; i++) {
        if (svs.clients[i].active)
            numCli++;
    }

    Con_Printf("serverprofile: %2i clients %2i msec\n", numCli, m);
}

//============================================================================


// "VCR1"

void Host::InitVCR(QuakeParms_p parms) {

    if (COM_CheckParm("-playback")) {
        if (com.argc != 2)      Host_SysError("No other parameters allowed with -playback\n");

        Sys_FileOpenRead("quake.vcr", &vcrFile);
        if (vcrFile == -1)      Host_SysError("playback file not found\n");

        {
            uint32_t sig;
            Sys_FileRead(vcrFile, &sig, sizeof(int));
            if (sig != VCR_SIGNATURE) Host_SysError("Invalid signature in vcr file\n");
        }

        Sys_FileRead(vcrFile, &com.argc, sizeof(int));
        com.argv = (cStringArray)malloc(com.argc * sizeof(cString));
        com.argv[0] = parms->argv[0];
        for (int i = 0; i < com.argc; i++) {
            int len;
            Sys_FileRead(vcrFile, &len, sizeof(int));
            cString p = (cString)malloc(len);
            Sys_FileRead(vcrFile, p, len);
            com.argv[i + 1] = p;
        }
        com.argc++; /* add one for arg[0] */
        parms->argc = com.argc;
        parms->argv = com.argv;
    }

    int n = COM_CheckParm("-record");
    if ((n) != 0) {
        vcrFile = Sys_FileOpenWrite("quake.vcr");

        {
            uint32_t sig = VCR_SIGNATURE;
            Sys_FileWrite(vcrFile, &sig, sizeof(int));
        }

        {
            int i = com.argc - 1;
            Sys_FileWrite(vcrFile, &i, sizeof(int));
            for (int i = 1; i < com.argc; i++) {
                if (i == n) {
                    int len = 10;
                    Sys_FileWrite(vcrFile, &len, sizeof(int));
                    Sys_FileWrite(vcrFile, (cString)"-playback", len);
                    continue;
                }
                int len = Q_strlen(com.argv[i]) + 1;
                Sys_FileWrite(vcrFile, &len, sizeof(int));
                Sys_FileWrite(vcrFile, com.argv[i], len);
            }
        }
    }

}

/*
====================
Host::Init
====================
*/
void Host::Init(QuakeParms_p parms) {
    minimum_memory = (standard_quake) ? MINIMUM_MEMORY : MINIMUM_MEMORY_LEVELPAK;

    if (COM_CheckParm("-minmemory"))
        parms->memsize = minimum_memory;

    host_parms = *parms;

    if (parms->memsize < minimum_memory)
        Host_SysError("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);

    com.argc = parms->argc;
    com.argv = parms->argv;

    Memory_Init(parms->membase, parms->memsize);
    Cbuf_Init();
    Cmd_Init();
    V_Init();
    Chase_Init();
    InitVCR(parms);
    COM_Init(parms->baseDir);
    InitLocal();
    W_LoadWadFile("gfx.wad");
    Key_Init();
    Con_Init();
    M_Init();
    PR_Init();
    Mod_Init();
    NET_Init();
    SV_Init();

    Con_Printf("Exe: " __TIME__ " " __DATE__ "\n");
    Con_Printf("%4.1f megabyte heap\n", parms->memsize / (1024 * 1024.0));

    R_InitTextures();  // needed even for dedicated servers

    if (cls.state != ca_dedicated) {
        host_basepal = (uint8_p)COM_LoadHunkFile("gfx/palette.lmp");
        if (!host_basepal)      Host_SysError("Couldn't load gfx/palette.lmp");
        host_colormap = (uint8_p)COM_LoadHunkFile("gfx/colormap.lmp");
        if (!host_colormap)     Host_SysError("Couldn't load gfx/colormap.lmp");

#ifndef _WIN32 // on non win32, mouse comes before video for security reasons
        IN_Init();
#endif
        VID_Init(host_basepal);

        Draw_Init();
        SCR_Init();
        R_Init();
#ifndef _WIN32
        // on Win32, sound initialization has to come before video initialization, so we
        // can put up a popup if the sound hardware is in use
        S_Init();
#else

#ifdef GLQUAKE
        // FIXME: doesn't use the new one-window approach yet
        S_Init();
#endif

#endif // _WIN32
        CDAudio_Init();
        Sbar_Init();
        CL_Init();
#ifdef _WIN32 // on non win32, mouse comes before video for security reasons
        IN_Init();
#endif
    }

    Cbuf_InsertText("exec quake.rc\n");

    Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
    host_hunklevel = Hunk_LowMark();

    host_initialized = true;

    Host_Printf("========Quake Initialized=========\n");
}


/*
===============
Host::Shutdown

FIXME: this is a callback from Sys_Quit and Host_SysError.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host::Shutdown() {
    static bool _isDown = false;

    if (_isDown) { printf("recursive shutdown\n"); return; }

    _isDown = true;

    // keep Con_Printf from trying to update the screen
    scr.disabled_for_loading = true;

    WriteConfiguration();

    CDAudio_Shutdown();
    NET_Shutdown();
    S_Shutdown();
    IN_Shutdown();

    if (cls.state != ca_dedicated) { VID_Shutdown(); }
}

bool Host::IsDedicated() {
    return cls.state == ca_dedicated;
}

bool Host::IsServerActive() {
    return sv.active;
}

#include "host_cmd.h"
/*
==================
Host::InitCommands
==================
*/
void Host::InitCommands() {
    Cmd_AddCommand("quit", Host_Quit_f);        // host
    Cmd_AddCommand("version", Host_Version_f);  // host

    Cmd_AddCommand("map", Host_Map_f);                  // sv
    Cmd_AddCommand("changelevel", Host_Changelevel_f);  // sv
#ifdef QUAKE2
    Cmd_AddCommand("changelevel2", Host_Changelevel2_f);    // sv
#endif
    Cmd_AddCommand("status", Host_Status_f);    // sv
#ifdef IDGODS
    Cmd_AddCommand("please", Host_Please_f);
#endif
    Cmd_AddCommand("kick", Host_Kick_f);        // sv
    Cmd_AddCommand("ping", Host_Ping_f);        // net
    Cmd_AddCommand("load", Host_Loadgame_f);    // sv
    Cmd_AddCommand("save", Host_Savegame_f);    // sv
    Cmd_AddCommand("pause", Host_Pause_f);      // sv

    Cmd_AddCommand("begin", Host_Begin_f);          // sv\cl
    Cmd_AddCommand("prespawn", Host_PreSpawn_f);    // sv\cl
    Cmd_AddCommand("spawn", Host_Spawn_f);          // sv\cl
    Cmd_AddCommand("connect", Host_Connect_f);      // cl
    Cmd_AddCommand("reconnect", Host_Reconnect_f);  // cl

    Cmd_AddCommand("give", Host_Give_f);            // cl
    Cmd_AddCommand("god", Host_God_f);              // sv
    Cmd_AddCommand("notarget", Host_Notarget_f);    // sv
    Cmd_AddCommand("fly", Host_Fly_f);              // sv
    Cmd_AddCommand("restart", Host_Restart_f);      // sv
    Cmd_AddCommand("name", Host_Name_f);            // cl
    Cmd_AddCommand("noclip", Host_Noclip_f);        // sv
    Cmd_AddCommand("say", Host_Say_f);              // cl
    Cmd_AddCommand("say_team", Host_Say_Team_f);    // cl
    Cmd_AddCommand("tell", Host_Tell_f);            // cl
    Cmd_AddCommand("color", Host_Color_f);          // cl
    Cmd_AddCommand("kill", Host_Kill_f);            // cl

    Cmd_AddCommand("startdemos", Host_Startdemos_f);    // cl
    Cmd_AddCommand("demos", Host_Demos_f);              // cl
    Cmd_AddCommand("stopdemo", Host_Stopdemo_f);        // cl

    Cmd_AddCommand("viewmodel", Host_Viewmodel_f);  // prog
    Cmd_AddCommand("viewframe", Host_Viewframe_f);  // prog
    Cmd_AddCommand("viewnext", Host_Viewnext_f);    // prog
    Cmd_AddCommand("viewprev", Host_Viewprev_f);    // prog

    Cmd_AddCommand("mcache", Mod_Print);
}

Host host;
