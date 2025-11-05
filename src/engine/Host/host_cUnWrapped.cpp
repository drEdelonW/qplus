#include "host.h"
#include "host.hpp"

#include <stdarg.h>
#include "server.h"
#undef SERVER
#include "client.h"
#include "sys.h"
#include "protocol.h"
#include "console.h"
#include "screen.h"
#include "msg.h"

#include <setjmp.h>
extern jmp_buf host_abortserver;

/*
================
Host.EndGame
================
*/
void Host_EndGame(cString message, ...) {
    va_list argptr;     va_start(argptr, message);
    char string[1024];  vsnprintf(string, sizeof(string), message, argptr);
    va_end(argptr);

    Con_DPrintf("Host.EndGame: %s\n", string);

    if (sv.active)                  host.ShutdownServer(false);
    if (cls.state == ca_dedicated)  Sys_Error("Host.EndGame: %s\n", string); // dedicated servers exit

    if (cls.demonum != -1)  CL_NextDemo();
    else                    CL_Disconnect();

    longjmp(host_abortserver, 1);
}

/*
================
Host.Error

This shuts down both the client and server
================
*/
void Host_Error(cString error, ...) {
    static bool inerror = false;
    if (inerror)    Sys_Error("Host.Error: recursively entered");
    inerror = true;

    SCR_EndLoadingPlaque();  // reenable screen updates

    va_list argptr;     va_start(argptr, error);
    char string[1024];  vsnprintf(string, sizeof(string), error, argptr);
    va_end(argptr);

    Con_Printf("Host.Error: %s\n", string);

    if (sv.active)                  host.ShutdownServer(false);
    if (cls.state == ca_dedicated)  Sys_Error("Host.Error: %s\n", string); // dedicated servers exit

    CL_Disconnect();
    cls.demonum = -1;

    inerror = false;

    longjmp(host_abortserver, 1);
}

/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf(cStringRO fmt, ...) {
    va_list argptr;     va_start(argptr, fmt);
    char string[1024];  vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);

    sizebuf_p pBuf = &remoteClient->message;
    MSG_WriteByte(pBuf, svc_print); MSG_WriteString(pBuf, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf(cString fmt, ...) {
    va_list argptr;     va_start(argptr, fmt);
    char string[1024];  vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);

    for (int i = 0; i < svs.maxClients; i++)
        if (svs.clients[i].active && svs.clients[i].spawned) {
            sizebuf_p pBuf = &svs.clients[i].message;
            MSG_WriteByte(pBuf, svc_print); MSG_WriteString(pBuf, string);
        }
}

/*
=================
Host.ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands(cString fmt, ...) {
    va_list argptr;     va_start(argptr, fmt);
    char string[1024];  vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);

    sizebuf_p pBuf = &remoteClient->message;
    MSG_WriteByte(pBuf, svc_stufftext); MSG_WriteString(pBuf, string);
}
