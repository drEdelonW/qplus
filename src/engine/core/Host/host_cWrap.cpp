#include "host.h"
#include "host.hpp"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "d_iface.h"
#include "server.h"
#include "common.h"
#include "sys.h"
#include "protocol.h"
#include "keys.h"
#include "console.h"
#include "vid.h"
#include "cmd.h"
#include "input.h"
#include "sound.h"
#include "cdaudio.h"
#include "wad.h"
#include "view.h"
#include "draw.h"
#include "menu.h"
#include "sbar.h"
#include "chase.h"
#include "r_local.h"
#include "mathlib.h"
#include "screen.h"
#include "q_tools.h"
#include "msg.h"


// QuakeParms_t host_parms;
// bool host_initialized;  // true if into command execution
// double host_frametime;
// double host_time;
// double realtime;    // without any filtering or bounding
// double oldrealtime;   // last frame run
// int32_t host_framecount;
// int host_hunklevel;
// int minimum_memory;
// client_p host_client;   // current client
// jmp_buf  host_abortserver;
// uint8_p host_basepal;
// uint8_p host_colormap;



/*
================
Host_EndGame
================
*/
void Host_EndGame(cString message, ...) {
    va_list  argptr;
    char  string[1024];

    va_start(argptr, message);
    vsprintf(string, message, argptr);
    va_end(argptr);
    Con_DPrintf("Host_EndGame: %s\n", string);

    if (sv.active)  Host_ShutdownServer(false);

    if (cls.state == ca_dedicated)  Sys_Error("Host_EndGame: %s\n", string); // dedicated servers exit

    if (cls.demonum != -1)  CL_NextDemo();
    else                    CL_Disconnect();

    longjmp(host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error(cString error, ...) {
    static bool inerror = false;
    va_list  argptr;
    char  string[1024];

    if (inerror)    Sys_Error("Host_Error: recursively entered");
    inerror = true;

    SCR_EndLoadingPlaque();  // reenable screen updates

    va_start(argptr, error);
    vsprintf(string, error, argptr);
    va_end(argptr);
    Con_Printf("Host_Error: %s\n", string);

    if (sv.active)  Host_ShutdownServer(false);

    if (cls.state == ca_dedicated)  Sys_Error("Host_Error: %s\n", string); // dedicated servers exit

    CL_Disconnect();
    cls.demonum = -1;

    inerror = false;

    longjmp(host_abortserver, 1);
}

/*
================
Host_FindMaxClients
================
*/
void Host_FindMaxClients() {
    host.FindMaxClients();
}


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal() {
    host.InitLocal();
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration() {
    host.WriteConfiguration();
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf(cStringRO fmt, ...) {
    va_list  argptr;
    char  string[1024];

    va_start(argptr, fmt);
    vsprintf(string, fmt, argptr);
    va_end(argptr);

    MSG_WriteByte(&host_client->message, svc_print);
    MSG_WriteString(&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf(cString fmt, ...) {
    va_list  argptr;
    char  string[1024];

    va_start(argptr, fmt);
    vsprintf(string, fmt, argptr);
    va_end(argptr);

    for (int i = 0; i < svs.maxclients; i++)
        if (svs.clients[i].active && svs.clients[i].spawned) {
            MSG_WriteByte(&svs.clients[i].message, svc_print);
            MSG_WriteString(&svs.clients[i].message, string);
        }
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands(cString fmt, ...) {
    va_list  argptr;
    char  string[1024];

    va_start(argptr, fmt);
    vsprintf(string, fmt, argptr);
    va_end(argptr);

    MSG_WriteByte(&host_client->message, svc_stufftext);
    MSG_WriteString(&host_client->message, string);
}


/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(bool crash) {
    host.ShutdownServer(crash);
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory() {
    host.ClearMemory();
}


//============================================================================


/*
===================
Host_FilterTime

Returns false if the time is too int16_t to run a frame
===================
*/
bool Host_FilterTime(float time) {
    return host.FilterTime(time);
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands() {
    host.GetConsoleCommands();
}


/*
==================
Host_ServerFrame

==================
*/
#ifdef FPS_20

void _Host_ServerFrame() {
    // run the world state
    pr_global_struct->frametime = host_frametime;

    // read client messages
    SV_RunClients();

    // move things around and think
    // always pause in single player if in console or menus
    if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game))
        SV_Physics();
}

void Host_ServerFrame() {
    // run the world state
    pr_global_struct->frametime = host_frametime;

    // set the time and clear the general datagram
    SV_ClearDatagram();

    // check for new clients
    SV_CheckForNewClients();

    float save_host_frametime;
    float temp_host_frametime = save_host_frametime = host_frametime;
    while (temp_host_frametime > (1.0 / 72.0)) {
        if (temp_host_frametime > 0.05)
            host_frametime = 0.05;
        else
            host_frametime = temp_host_frametime;
        temp_host_frametime -= host_frametime;
        _Host_ServerFrame();
    }
    host_frametime = save_host_frametime;

    // send all messages to the clients
    SV_SendClientMessages();
}

#else

void Host_ServerFrame() {
    host.ServerFrame();
}

#endif


/*
==================
Host_Frame

Runs all active servers
==================
*/

void Host_Frame(float time) {
    host.Frame(time);
}

//============================================================================


extern int vcrFile;
#define VCR_SIGNATURE 0x56435231
// "VCR1"

void Host_InitVCR(QuakeParms_p parms) {
    host.InitVCR(parms);
}

/*
====================
Host_Init
====================
*/
void Host_Init(QuakeParms_p parms) {
    host.Init(parms);
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown() {
  host.Shutdown();
}

