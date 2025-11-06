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
// net_main.c

#include "host.h"
#include "server.h"
#undef SERVER
#include "common.h"
#include "sys.h"
#include "cmd.h"
#include "cbuf.h"
#include "net_vcr.h"
#include "cvar_q1.h"
#include "console.h"
#include "q_tools.h"


qsocket_p net_activeSockets = NULL;
qsocket_p net_freeSockets = NULL;
int32_t net_numsockets = 0;

bool serialAvailable = false;
bool ipxAvailable = false;
bool tcpipAvailable = false;

int32_t net_hostport;
int32_t DEFAULTnet_hostport = 26000;

char my_ipx_address[NET_NAMELEN];
char my_tcpip_address[NET_NAMELEN];

void (*GetComPortConfig) (int32_t portNumber, int32_p port, int32_p irq, int32_p baud, bool* useModem);
void (*SetComPortConfig) (int32_t portNumber, int32_t port, int32_t irq, int32_t baud, bool useModem);
void (*GetModemConfig) (int32_t portNumber, cString dialType, cString clear, cString init, cString hangup);
void (*SetModemConfig) (int32_t portNumber, cString dialType, cString clear, cString init, cString hangup);

static bool  listening = false;

bool slistInProgress = false;
bool slistSilent = false;
bool slistLocal = true;
static double slistStartTime;
static int slistLastShown;

static void Slist_Send();
static void Slist_Poll();
PollProcedure slistSendProcedure = { NULL, 0.0, Slist_Send };
PollProcedure slistPollProcedure = { NULL, 0.0, Slist_Poll };


sizebuf_t net_message;
int32_t net_activeconnections = 0;

int32_t messagesSent = 0;
int32_t messagesReceived = 0;
int32_t unreliableMessagesSent = 0;
int32_t unreliableMessagesReceived = 0;

bool configRestored = false;

int  vcrFile = -1;
bool recording = false;

// these two macros are to make the code more readable
#define sfunc net_drivers[sock->driver]
#define dfunc net_drivers[net_driverlevel]

int32_t  net_driverlevel;


double net_time;

double SetNetTime() {
    net_time = Sys_FloatTime();
    return net_time;
}


/*
===================
NET_NewQSocket

Called by drivers when a new communications endpoint is required
The sequence and buffer fields will be filled in properly
===================
*/
qsocket_p NET_NewQSocket() {
    if (net_freeSockets == NULL)
        return NULL;

    if (net_activeconnections >= svs.maxClients)
        return NULL;

    // get one from free list
    qsocket_p sock = net_freeSockets;
    net_freeSockets = sock->next;

    // add it to active list
    sock->next = net_activeSockets;
    net_activeSockets = sock;

    sock->disconnected = false;
    sock->connecttime = net_time;
    Q_strcpy(sock->address, "UNSET ADDRESS");
    sock->driver = net_driverlevel;
    sock->socket = 0;
    sock->driverdata = NULL;
    sock->canSend = true;
    sock->sendNext = false;
    sock->lastMessageTime = net_time;
    sock->ackSequence = 0;
    sock->sendSequence = 0;
    sock->unreliableSendSequence = 0;
    sock->sendMessageLength = 0;
    sock->receiveSequence = 0;
    sock->unreliableReceiveSequence = 0;
    sock->receiveMessageLength = 0;

    return sock;
}


void NET_FreeQSocket(qsocket_p sock) {
    // remove it from active list
    if (sock == net_activeSockets)
        net_activeSockets = net_activeSockets->next;
    else {
        qsocket_p s = net_activeSockets;
        for (; s; s = s->next)
            if (s->next == sock) {
                s->next = sock->next;
                break;
            }
        if (!s)
            Sys_Error("NET_FreeQSocket: not active\n");
    }

    // add it to free list
    sock->next = net_freeSockets;
    net_freeSockets = sock;
    sock->disconnected = true;
}


static void NET_Listen_f() {
    if (Cmd_Argc() != 2) {
        Con_Printf("\"listen\" is \"%u\"\n", listening ? 1 : 0);
        return;
    }

    listening = Q_atoi(Cmd_Argv(1)) ? true : false;

    for (net_driverlevel = 0; net_driverlevel < net_numdrivers; net_driverlevel++) {
        if (net_drivers[net_driverlevel].initialized == false)
            continue;
        dfunc.Listen(listening);
    }
}


static void MaxPlayers_f() {
    if (Cmd_Argc() != 2) {
        Con_Printf("\"maxplayers\" is \"%u\"\n", svs.maxClients);
        return;
    }

    if (sv.active) {
        Con_Printf("maxplayers can not be changed while a server is running.\n");
        return;
    }

    int n = Q_atoi(Cmd_Argv(1));
    if (n < 1)
        n = 1;
    if (n > svs.maxClientsLimit) {
        n = svs.maxClientsLimit;
        Con_Printf("\"maxplayers\" set to \"%u\"\n", n);
    }

    if ((n == 1) && listening)      Cbuf_AddText("listen 0\n");
    if ((n > 1) && (!listening))    Cbuf_AddText("listen 1\n");

    svs.maxClients = n;
    if (n == 1) Cvar_Set("deathmatch", "0");
    else        Cvar_Set("deathmatch", "1");
}


static void NET_Port_f() {
    if (Cmd_Argc() != 2) {
        Con_Printf("\"port\" is \"%u\"\n", net_hostport);
        return;
    }

    int n = Q_atoi(Cmd_Argv(1));
    if ((n < 1) || (n > 65534)) {
        Con_Printf("Bad value, must be between 1 and 65534\n");
        return;
    }

    DEFAULTnet_hostport = n;
    net_hostport = n;

    if (listening) {
        // force a change to the new port
        Cbuf_AddText("listen 0\n");
        Cbuf_AddText("listen 1\n");
    }
}


static void PrintSlistHeader() {
    Con_Printf("Server          Map             Users\n");
    Con_Printf("--------------- --------------- -----\n");
    slistLastShown = 0;
}


static void PrintSlist() {
    int n = slistLastShown;
    for (; n < hostCacheCount; n++) {
        if (hostcache[n].maxusers)
            Con_Printf("%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
        else
            Con_Printf("%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
    }
    slistLastShown = n;
}


static void PrintSlistTrailer() {
    if (hostCacheCount)     Con_Printf("== end list ==\n\n");
    else                    Con_Printf("No Quake servers found.\n\n");
}


void NET_Slist_f() {
    if (slistInProgress)    return;

    if (!slistSilent) {
        Con_Printf("Looking for Quake servers...\n");
        PrintSlistHeader();
    }

    slistInProgress = true;
    slistStartTime = Sys_FloatTime();

    SchedulePollProcedure(&slistSendProcedure, 0.0);
    SchedulePollProcedure(&slistPollProcedure, 0.1);

    hostCacheCount = 0;
}


static void Slist_Send() {
    for (net_driverlevel = 0; net_driverlevel < net_numdrivers; net_driverlevel++) {
        if ((!slistLocal && (net_driverlevel == 0)) ||
            (net_drivers[net_driverlevel].initialized == false))
            continue;
        dfunc.SearchForHosts(true);
    }

    if ((Sys_FloatTime() - slistStartTime) < 0.5)
        SchedulePollProcedure(&slistSendProcedure, 0.75);
}


static void Slist_Poll() {
    for (net_driverlevel = 0; net_driverlevel < net_numdrivers; net_driverlevel++) {
        if ((!slistLocal && (net_driverlevel == 0)) ||
            (net_drivers[net_driverlevel].initialized == false))
            continue;
        dfunc.SearchForHosts(false);
    }

    if (!slistSilent)   PrintSlist();

    if ((Sys_FloatTime() - slistStartTime) < 1.5) {
        SchedulePollProcedure(&slistPollProcedure, 0.1);
        return;
    }

    if (!slistSilent)
        PrintSlistTrailer();
    slistInProgress = false;
    slistSilent = false;
    slistLocal = true;
}


/*
===================
NET_Connect
===================
*/

int32_t hostCacheCount = 0;
hostcache_t hostcache[HOSTCACHESIZE];

qsocket_p NET_Connect(cString host) {
    int numdrivers = net_numdrivers;

    SetNetTime();

    if (host && (*host == 0))
        host = NULL;

    if (host) {
        if (Q_strcasecmp(host, "local") == 0) {
            numdrivers = 1;
            goto JustDoIt;
        }

        if (hostCacheCount) {
            int n = 0;
            for (; n < hostCacheCount; n++)
                if (Q_strcasecmp(host, hostcache[n].name) == 0) {
                    host = hostcache[n].cname;
                    break;
                }
            if (n < hostCacheCount)
                goto JustDoIt;
        }
    }

    slistSilent = host ? true : false;
    NET_Slist_f();

    while (slistInProgress)
        NET_Poll();

    if (host == NULL) {
        if (hostCacheCount != 1)
            return NULL;
        host = hostcache[0].cname;
        Con_Printf("Connecting to...\n%s @ %s\n\n", hostcache[0].name, host);
    }

    if (hostCacheCount)
        for (int n = 0; n < hostCacheCount; n++)
            if (Q_strcasecmp(host, hostcache[n].name) == 0) {
                host = hostcache[n].cname;
                break;
            }

JustDoIt:
    for (net_driverlevel = 0; net_driverlevel < numdrivers; net_driverlevel++) {
        if (net_drivers[net_driverlevel].initialized == false)
            continue;
        qsocket_p ret = dfunc.Connect(host);
        if (ret)
            return ret;
    }

    if (host) {
        Con_Printf("\n");
        PrintSlistHeader();
        PrintSlist();
        PrintSlistTrailer();
    }

    return NULL;
}


/*
===================
NET_CheckNewConnections
===================
*/

struct {
    double time;
    vcr_opcode_t op;
    int32_t  session;
} vcrConnect;

qsocket_p NET_CheckNewConnections() {
    SetNetTime();

    for (net_driverlevel = 0; net_driverlevel < net_numdrivers; net_driverlevel++) {
        if ((net_drivers[net_driverlevel].initialized == false) ||
            (net_driverlevel && (listening == false)))
            continue;
        qsocket_p ret = dfunc.CheckNewConnections();
        if (ret) {
            if (recording) {
                vcrConnect.time = host_time;
                vcrConnect.op = VCR_OP_CONNECT;
                vcrConnect.session = (intptr_t)ret;
                Sys_FileWrite(vcrFile, &vcrConnect, sizeof(vcrConnect));
                Sys_FileWrite(vcrFile, ret->address, NET_NAMELEN);
            }
            return ret;
        }
    }

    if (recording) {
        vcrConnect.time = host_time;
        vcrConnect.op = VCR_OP_CONNECT;
        vcrConnect.session = 0;
        Sys_FileWrite(vcrFile, &vcrConnect, sizeof(vcrConnect));
    }

    return NULL;
}

/*
===================
NET_Close
===================
*/
void NET_Close(qsocket_p sock) {
    if ((!sock) ||
        (sock->disconnected))
        return;

    SetNetTime();

    // call the driver_Close function
    sfunc.Close(sock);

    NET_FreeQSocket(sock);
}


/*
=================
NET_GetMessage

If there is a complete message, return it in net_message

returns 0 if no data is waiting
returns 1 if a message was received
returns -1 if connection is invalid
=================
*/

struct {
    double time;
    vcr_opcode_t op;
    int32_t session;
    int32_t ret;
    int32_t len;
} vcrGetMessage;


int32_t  NET_GetMessage(qsocket_p sock) {
    if (!sock)
        return -1;

    if (sock->disconnected) {
        Con_Printf("NET_GetMessage: disconnected socket\n");
        return -1;
    }

    SetNetTime();

    int32_t ret = sfunc.QGetMessage(sock);

    // see if this connection has timed out
    if ((ret == 0) && (sock->driver)) {
        if (net_time - sock->lastMessageTime > net_messagetimeout.value) {
            NET_Close(sock);
            return -1;
        }
    }


    if (ret > 0) {
        if (sock->driver) {
            sock->lastMessageTime = net_time;
            if (ret == 1)       messagesReceived++;
            else if (ret == 2)  unreliableMessagesReceived++;
        }

        if (recording) {
            vcrGetMessage.time = host_time;
            vcrGetMessage.op = VCR_OP_GETMESSAGE;
            vcrGetMessage.session = (intptr_t)sock;
            vcrGetMessage.ret = ret;
            vcrGetMessage.len = net_message.cursize;
            Sys_FileWrite(vcrFile, &vcrGetMessage, 24);
            Sys_FileWrite(vcrFile, net_message.data, net_message.cursize);
        }
    }
    else {
        if (recording) {
            vcrGetMessage.time = host_time;
            vcrGetMessage.op = VCR_OP_GETMESSAGE;
            vcrGetMessage.session = (intptr_t)sock;
            vcrGetMessage.ret = ret;
            Sys_FileWrite(vcrFile, &vcrGetMessage, 20);
        }
    }

    return ret;
}


/*
==================
NET_SendMessage

Try to send a complete length+message unit over the reliable stream.
returns 0 if the message cannot be delivered reliably, but the connection
        is still considered valid
returns 1 if the message was sent properly
returns -1 if the connection died
==================
*/
struct
{
    double  time;
    vcr_opcode_t op;
    int32_t  session;
    int r;
} vcrSendMessage;

int32_t NET_SendMessage(qsocket_p sock, sizebuf_p data) {
    if (!sock)
        return -1;

    if (sock->disconnected) {
        Con_Printf("NET_SendMessage: disconnected socket\n");
        return -1;
    }

    SetNetTime();
    int32_t r = sfunc.QSendMessage(sock, data);
    if ((r == 1) && sock->driver)
        messagesSent++;

    if (recording) {
        vcrSendMessage.time = host_time;
        vcrSendMessage.op = VCR_OP_SENDMESSAGE;
        vcrSendMessage.session = (intptr_t)sock;
        vcrSendMessage.r = r;
        Sys_FileWrite(vcrFile, &vcrSendMessage, 20);
    }

    return r;
}


int32_t NET_SendUnreliableMessage(qsocket_p sock, sizebuf_p data) {
    if (!sock)
        return -1;

    if (sock->disconnected) {
        Con_Printf("NET_SendMessage: disconnected socket\n");
        return -1;
    }

    SetNetTime();
    int32_t r = sfunc.SendUnreliableMessage(sock, data);
    if ((r == 1) && sock->driver)
        unreliableMessagesSent++;

    if (recording) {
        vcrSendMessage.time = host_time;
        vcrSendMessage.op = VCR_OP_SENDMESSAGE;
        vcrSendMessage.session = (intptr_t)sock;
        vcrSendMessage.r = r;
        Sys_FileWrite(vcrFile, &vcrSendMessage, 20);
    }

    return r;
}


/*
==================
NET_CanSendMessage

Returns true or false if the given qsocket can currently accept a
message to be transmitted.
==================
*/
bool NET_CanSendMessage(qsocket_p sock) {
    if ((!sock) ||
        (sock->disconnected))
        return false;

    SetNetTime();

    int32_t r = sfunc.CanSendMessage(sock);

    if (recording) {
        vcrSendMessage.time = host_time;
        vcrSendMessage.op = VCR_OP_CANSENDMESSAGE;
        vcrSendMessage.session = (intptr_t)sock;
        vcrSendMessage.r = r;
        Sys_FileWrite(vcrFile, &vcrSendMessage, 20);
    }

    return r;
}


int32_t NET_SendToAll(sizebuf_p data, int32_t blocktime) {
    int32_t count = 0;
    bool state1[MAX_SCOREBOARD];
    bool state2[MAX_SCOREBOARD];

    remoteClient = svs.clients;
    for (int32_t i = 0; i < svs.maxClients; i++, remoteClient++) {
        if (!remoteClient->netconnection)
            continue;
        if (remoteClient->active) {
            if (remoteClient->netconnection->driver == 0) {
                NET_SendMessage(remoteClient->netconnection, data);
                state1[i] = true;
                state2[i] = true;
                continue;
            }
            count++;
            state1[i] = false;
            state2[i] = false;
        }
        else {
            state1[i] = true;
            state2[i] = true;
        }
    }

    double start = Sys_FloatTime();
    while (count) {
        count = 0;
        remoteClient = svs.clients;
        for (int32_t i = 0; i < svs.maxClients; i++, remoteClient++) {
            if (!state1[i]) {
                if (NET_CanSendMessage(remoteClient->netconnection)) {
                    state1[i] = true;
                    NET_SendMessage(remoteClient->netconnection, data);
                }
                else {
                    NET_GetMessage(remoteClient->netconnection);
                }
                count++;
                continue;
            }

            if (!state2[i]) {
                if (NET_CanSendMessage(remoteClient->netconnection)) {
                    state2[i] = true;
                }
                else {
                    NET_GetMessage(remoteClient->netconnection);
                }
                count++;
                continue;
            }
        }
        if ((Sys_FloatTime() - start) > blocktime)
            break;
    }
    return count;
}


//=============================================================================

/*
====================
NET_Init
====================
*/
#include "client.h"
void NET_Init() {
    if (COM_CheckParm("-playback")) {
        net_numdrivers = 1;
        net_drivers[0].Init = VCR_Init;
    }

    if (COM_CheckParm("-record"))
        recording = true;

    int param = COM_CheckParm("-port");
    if (!param)     param = COM_CheckParm("-udpport");
    if (!param)     param = COM_CheckParm("-ipxport");

    if (param) {
        if (param < com.argc - 1)
            DEFAULTnet_hostport = Q_atoi(com.argv[param + 1]);
        else
            Sys_Error("NET_Init: you must specify a number after -port");
    }
    net_hostport = DEFAULTnet_hostport;

    if (COM_CheckParm("-listen") || (cls.state == ca_dedicated))
        listening = true;
    net_numsockets = svs.maxClientsLimit;
    if (cls.state != ca_dedicated)
        net_numsockets++;

    SetNetTime();

    for (int i = 0; i < net_numsockets; i++) {
        qsocket_p s = (qsocket_p)Hunk_AllocName(sizeof(qsocket_t), "qsocket");
        s->next = net_freeSockets;
        net_freeSockets = s;
        s->disconnected = true;
    }

    // allocate space for network message buffer
    SZ_Alloc(&net_message, NET_MAXMESSAGE);

    Cvar_RegisterVariable(&net_messagetimeout);
    Cvar_RegisterVariable(&hostname);
    Cvar_RegisterVariable(&config_com_port);
    Cvar_RegisterVariable(&config_com_irq);
    Cvar_RegisterVariable(&config_com_baud);
    Cvar_RegisterVariable(&config_com_modem);
    Cvar_RegisterVariable(&config_modem_dialtype);
    Cvar_RegisterVariable(&config_modem_clear);
    Cvar_RegisterVariable(&config_modem_init);
    Cvar_RegisterVariable(&config_modem_hangup);
#ifdef IDGODS
    Cvar_RegisterVariable(&idgods);
#endif

    Cmd_AddCommand("slist", NET_Slist_f);
    Cmd_AddCommand("listen", NET_Listen_f);
    Cmd_AddCommand("maxplayers", MaxPlayers_f);
    Cmd_AddCommand("port", NET_Port_f);

    // initialize all the drivers
    for (net_driverlevel = 0; net_driverlevel < net_numdrivers; net_driverlevel++) {
        int controlSocket = net_drivers[net_driverlevel].Init();
        if (controlSocket == -1)
            continue;
        net_drivers[net_driverlevel].initialized = true;
        net_drivers[net_driverlevel].controlSock = controlSocket;
        if (listening)
            net_drivers[net_driverlevel].Listen(true);
    }

    if (*my_ipx_address)    Con_DPrintf("IPX address %s\n", my_ipx_address);
    if (*my_tcpip_address)  Con_DPrintf("TCP/IP address %s\n", my_tcpip_address);
}

/*
====================
NET_Shutdown
====================
*/

void NET_Shutdown() {
    SetNetTime();

    for (qsocket_p sock = net_activeSockets; sock; sock = sock->next)
        NET_Close(sock);

    //
    // shutdown the drivers
    //
    for (net_driverlevel = 0; net_driverlevel < net_numdrivers; net_driverlevel++) {
        if (net_drivers[net_driverlevel].initialized == true) {
            net_drivers[net_driverlevel].Shutdown();
            net_drivers[net_driverlevel].initialized = false;
        }
    }

    if (vcrFile != -1) {
        Con_Printf("Closing vcrfile.\n");
        Sys_FileClose(vcrFile);
    }
}


static PollProcedure* pollProcedureList = NULL;

void NET_Poll() {
    if (!configRestored) {
        if (serialAvailable) {
            bool useModem;
            if (config_com_modem.value == 1.0)
                useModem = true;
            else
                useModem = false;
            SetComPortConfig(0, (int)config_com_port.value, (int)config_com_irq.value, (int)config_com_baud.value, useModem);
            SetModemConfig(0, config_modem_dialtype.string, config_modem_clear.string, config_modem_init.string, config_modem_hangup.string);
        }
        configRestored = true;
    }

    SetNetTime();

    for (PollProcedure* pp = pollProcedureList; pp; pp = pp->next) {
        if (pp->nextTime > net_time)
            break;
        pollProcedureList = pp->next;
        pp->procedure(
            // pp->arg
        );
    }
}


void SchedulePollProcedure(PollProcedure* proc, double timeOffset) {
    proc->nextTime = Sys_FloatTime() + timeOffset;
    PollProcedure* prev = NULL;
    PollProcedure* pp = pollProcedureList;
    for (; pp; pp = pp->next) {
        if (pp->nextTime >= proc->nextTime)
            break;
        prev = pp;
    }

    if (prev == NULL) {
        proc->next = pollProcedureList;
        pollProcedureList = proc;
        return;
    }

    proc->next = pp;
    prev->next = proc;
}


#ifdef IDGODS
#define IDNET  0xc0f62800

bool IsID(struct qsockaddr* addr) {
    if ((idgods.value == 0.0) ||
        (addr->sa_family != 2))
        return false;

    if ((BigLong(*(int*)&addr->sa_data[2]) & 0xffffff00) == IDNET)
        return true;
    return false;
}
#endif
