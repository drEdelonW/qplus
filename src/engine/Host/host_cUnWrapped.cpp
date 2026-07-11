#include "host.h"
#include "host.hpp"

// #include <stdarg.h>
#include "VA.h"
#include "server.h"
#undef SERVER   // TODO: remove this workaround
#include "client.h"
#include "sys.h"
#include "protocol.h"
#include "console.h"
#include "screen.h"
#include "msg.h"
#include "GameRule.h"

#include <setjmp.h>
extern jmp_buf host_abortserver;

/*
================
Host.EndGame
================
*/
void Host_EndGame(cString message, ...) {
    VaBuff_t string;
    VA_EXPAND(string, message);
    Con_DPrintf("Host.EndGame: %s\n", string);

    if (SV_IsActive())                  host.ShutdownServer(false);
    if (cls.state == ca_dedicated)      Host_SysError("Host.EndGame: %s\n", string); // dedicated servers exit

    if (cls.demonum != -1)  CL_NextDemo();
    else                    CL_Disconnect();

    longjmp(host_abortserver, 1);
}

void Host_Printf(cStringRO fmt, ...) {
    VaBuff_t string;
    VA_EXPAND(string, fmt);
    Sys_Printf("%s", string);
}

/*
================
Host.Error

This shuts down both the client and server
================
*/
void Host_Error(cString error, ...) {
    static bool inerror = false;
    if (inerror)    Host_SysError("Host.Error: recursively entered");
    inerror = true;

    SCR_EndLoadingPlaque();  // reenable screen updates

    VaBuff_t string;
    VA_EXPAND(string, error);
    Con_Printf("Host.Error: %s\n", string);

    if (SV_IsActive())                  host.ShutdownServer(false);
    if (cls.state == ca_dedicated)  Host_SysError("Host.Error: %s\n", string); // dedicated servers exit

    CL_Disconnect();
    cls.demonum = -1;

    inerror = false;

    longjmp(host_abortserver, 1);
}

void Host_SysError(cStringRO error, ...) {
    VaBuff_t string;
    VA_EXPAND(string, error);
    Sys_Error("%s", string);
}

/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf(cStringRO fmt, ...) {
    VaBuff_t string;
    VA_EXPAND(string, fmt);
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
    VaBuff_t string;
    VA_EXPAND(string, fmt);
    for (int i = 0; i < GetSvMaxClients(); i++)
        if ((svs.clients[i].active) &&
            (svs.clients[i].spawned)
            ) {
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
    VaBuff_t string;
    VA_EXPAND(string, fmt);
    sizebuf_p pBuf = &remoteClient->message;
    MSG_WriteByte(pBuf, svc_stufftext); MSG_WriteString(pBuf, string);
}
