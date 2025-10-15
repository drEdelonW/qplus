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
// net_dgrm.c

// This is enables a simple IP banning mechanism
#include <stdint.h>
#include "cvar_q1.h"


#define BAN_TEST

#ifdef BAN_TEST
#   if defined(_WIN32)
#       include <windows.h>
#   elif defined (NeXT)
#       include <sys/socket.h>
#       include <arpa/inet.h>
#   else
#       include "types.h"

#       define AF_INET 		2	/* internet */
struct in_addr {
    union {
        struct { uint8_t s_b1, s_b2, s_b3, s_b4; } S_un_b;
        struct { uint16_t s_w1, s_w2; } S_un_w;
        uint32_t S_addr;
    } S_un;
};
#       define	s_addr	S_un.S_addr	/* can be used for most tcp & ip code */
struct sockaddr_in {
    int16_t sin_family;
    uint16_t sin_port;
    struct in_addr	sin_addr;
    char			sin_zero[8];
};
cString inet_ntoa(struct in_addr in);
uint32_t inet_addr(const cString cp);
#   endif
#endif	// BAN_TEST
typedef struct in_addr in_addr_t;
typedef in_addr_t* in_addr_p;

#include "quakedef.h"
#include "net_dgrm.h"

// these two macros are to make the code more readable
#define sfunc   net_landrivers[sock->landriver]
#define dfunc   net_landrivers[_net_landriverlevel]

static int _net_landriverlevel;

/* statistic counters */
int	packetsSent = 0;
int	packetsReSent = 0;
int packetsReceived = 0;
int receivedDuplicateCount = 0;
int shortPacketCount = 0;
int droppedDatagrams;

static int _myDriverLevel;

struct {
    uint32_t length;
    uint32_t sequence;
    uint8_t data[MAX_DATAGRAM];
} packetBuffer;

extern bool m_return_onerror;
extern char m_return_reason[32];


#ifdef DEBUG
cString StrAddr(qsockaddr_p addr) {
    static char buf[34];
    uint8_p p = (uint8_p)addr;

    for (int n = 0; n < 16; n++)
        sprintf(buf + n * 2, "%02x", *p++);
    return buf;
}
#endif


#ifdef BAN_TEST
uint32_t banAddr = 0x00000000;
uint32_t banMask = 0xFFFFFFFF;

void NET_Ban_f() {
    char addrStr[32];
    char maskStr[32];
    void (*print) (cString fmt, ...);

    if (cmd_source == src_command) {
        if (!sv.active) {
            Cmd_ForwardToServer();
            return;
        }
        print = Con_Printf;
    }
    else {
        if (pr_global_struct->deathmatch && !host_client->privileged)
            return;
        print = SV_ClientPrintf;
    }

    switch (Cmd_Argc()) {
    case 1:
        if (((in_addr_p)&banAddr)->s_addr) {
            Q_strcpy(addrStr, inet_ntoa(*(in_addr_p)&banAddr));
            Q_strcpy(maskStr, inet_ntoa(*(in_addr_p)&banMask));
            print("Banning %s [%s]\n", addrStr, maskStr);
        }
        else
            print("Banning not active\n");
        break;

    case 2:
        if (Q_strcasecmp(Cmd_Argv(1), "off") == 0)
            banAddr = 0x00000000;
        else
            banAddr = inet_addr(Cmd_Argv(1));
        banMask = 0xffffffff;
        break;

    case 3:
        banAddr = inet_addr(Cmd_Argv(1));
        banMask = inet_addr(Cmd_Argv(2));
        break;

    default:
        print("BAN ip_address [mask]\n");
        break;
    }
}
#endif


int Datagram_SendMessage(qsocket_p sock, sizebuf_p data) {
#ifdef DEBUG
    if (data->cursize == 0)
        Sys_Error("Datagram_SendMessage: zero length message\n");

    if (data->cursize > NET_MAXMESSAGE)
        Sys_Error("Datagram_SendMessage: message too big %u\n", data->cursize);

    if (sock->canSend == false)
        Sys_Error("SendMessage: called with canSend == false\n");
#endif

    Q_memcpy(sock->sendMessage, data->data, data->cursize);
    sock->sendMessageLength = data->cursize;
    uint32_t dataLen;
    uint32_t eom;

    if (data->cursize <= MAX_DATAGRAM) {
        dataLen = data->cursize;
        eom = NETFLAG_EOM;
    }
    else {
        dataLen = MAX_DATAGRAM;
        eom = 0;
    }
    uint32_t packetLen = NET_HEADERSIZE + dataLen;

    packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
    packetBuffer.sequence = BigLong(sock->sendSequence++);
    Q_memcpy(packetBuffer.data, sock->sendMessage, dataLen);

    sock->canSend = false;

    if (sfunc.Write(sock->socket, (uint8_p)&packetBuffer, packetLen, &sock->addr) == -1)
        return -1;

    sock->lastSendTime = net_time;
    packetsSent++;
    return 1;
}


int SendMessageNext(qsocket_p sock) {
    uint32_t dataLen;
    uint32_t eom;

    if (sock->sendMessageLength <= MAX_DATAGRAM) {
        dataLen = sock->sendMessageLength;
        eom = NETFLAG_EOM;
    }
    else {
        dataLen = MAX_DATAGRAM;
        eom = 0;
    }
    uint32_t packetLen = NET_HEADERSIZE + dataLen;

    packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
    packetBuffer.sequence = BigLong(sock->sendSequence++);
    Q_memcpy(packetBuffer.data, sock->sendMessage, dataLen);

    sock->sendNext = false;

    if (sfunc.Write(sock->socket, (uint8_p)&packetBuffer, packetLen, &sock->addr) == -1)
        return -1;

    sock->lastSendTime = net_time;
    packetsSent++;
    return 1;
}


int ReSendMessage(qsocket_p sock) {
    uint32_t dataLen;
    uint32_t eom;

    if (sock->sendMessageLength <= MAX_DATAGRAM) {
        dataLen = sock->sendMessageLength;
        eom = NETFLAG_EOM;
    }
    else {
        dataLen = MAX_DATAGRAM;
        eom = 0;
    }
    uint32_t packetLen = NET_HEADERSIZE + dataLen;

    packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
    packetBuffer.sequence = BigLong(sock->sendSequence - 1);
    Q_memcpy(packetBuffer.data, sock->sendMessage, dataLen);

    sock->sendNext = false;

    if (sfunc.Write(sock->socket, (uint8_p)&packetBuffer, packetLen, &sock->addr) == -1)
        return -1;

    sock->lastSendTime = net_time;
    packetsReSent++;
    return 1;
}


bool Datagram_CanSendMessage(qsocket_p sock) {
    if (sock->sendNext)
        SendMessageNext(sock);

    return sock->canSend;
}


bool Datagram_CanSendUnreliableMessage(qsocket_p sock) {
    return true;
}


int Datagram_SendUnreliableMessage(qsocket_p sock, sizebuf_p data) {
#ifdef DEBUG
    if (data->cursize == 0)
        Sys_Error("Datagram_SendUnreliableMessage: zero length message\n");

    if (data->cursize > MAX_DATAGRAM)
        Sys_Error("Datagram_SendUnreliableMessage: message too big %u\n", data->cursize);
#endif

    int packetLen = NET_HEADERSIZE + data->cursize;

    packetBuffer.length = BigLong(packetLen | NETFLAG_UNRELIABLE);
    packetBuffer.sequence = BigLong(sock->unreliableSendSequence++);
    Q_memcpy(packetBuffer.data, data->data, data->cursize);

    if (sfunc.Write(sock->socket, (uint8_p)&packetBuffer, packetLen, &sock->addr) == -1)
        return -1;

    packetsSent++;
    return 1;
}


int	Datagram_GetMessage(qsocket_p sock) {
    int	ret = 0;

    if (!sock->canSend)
        if ((net_time - sock->lastSendTime) > 1.0)
            ReSendMessage(sock);

    while (1) {
        qsockaddr_t readaddr;
        uint32_t length = sfunc.Read(sock->socket, (uint8_p)&packetBuffer, NET_DATAGRAMSIZE, &readaddr);

        //	if ((rand() & 255) > 220)
        //		continue;

        if (length == 0)
            break;

        if (length == -1) {
            Con_Printf("Read error\n");
            return -1;
        }

        if (sfunc.AddrCompare(&readaddr, &sock->addr) != 0) {
#ifdef DEBUG
            Con_DPrintf("Forged packet received\n");
            Con_DPrintf("Expected: %s\n", StrAddr(&sock->addr));
            Con_DPrintf("Received: %s\n", StrAddr(&readaddr));
#endif
            continue;
        }

        if (length < NET_HEADERSIZE) {
            shortPacketCount++;
            continue;
        }

        length = BigLong(packetBuffer.length);
        uint32_t flags = length & (~NETFLAG_LENGTH_MASK);
        length &= NETFLAG_LENGTH_MASK;

        if (flags & NETFLAG_CTL)
            continue;

        uint32_t sequence = BigLong(packetBuffer.sequence);
        packetsReceived++;

        if (flags & NETFLAG_UNRELIABLE) {
            if (sequence < sock->unreliableReceiveSequence) {
                Con_DPrintf("Got a stale datagram\n");
                ret = 0;
                break;
            }
            if (sequence != sock->unreliableReceiveSequence) {
                uint32_t count = sequence - sock->unreliableReceiveSequence;
                droppedDatagrams += count;
                Con_DPrintf("Dropped %u datagram(s)\n", count);
            }
            sock->unreliableReceiveSequence = sequence + 1;

            length -= NET_HEADERSIZE;

            SZ_Clear(&net_message);
            SZ_Write(&net_message, packetBuffer.data, length);

            ret = 2;
            break;
        }

        if (flags & NETFLAG_ACK) {
            if (sequence != (sock->sendSequence - 1)) {
                Con_DPrintf("Stale ACK received\n");
                continue;
            }
            if (sequence == sock->ackSequence) {
                sock->ackSequence++;
                if (sock->ackSequence != sock->sendSequence)
                    Con_DPrintf("ack sequencing error\n");
            }
            else {
                Con_DPrintf("Duplicate ACK received\n");
                continue;
            }
            sock->sendMessageLength -= MAX_DATAGRAM;
            if (sock->sendMessageLength > 0) {
                Q_memcpy(sock->sendMessage, sock->sendMessage + MAX_DATAGRAM, sock->sendMessageLength);
                sock->sendNext = true;
            }
            else {
                sock->sendMessageLength = 0;
                sock->canSend = true;
            }
            continue;
        }

        if (flags & NETFLAG_DATA) {
            packetBuffer.length = BigLong(NET_HEADERSIZE | NETFLAG_ACK);
            packetBuffer.sequence = BigLong(sequence);
            sfunc.Write(sock->socket, (uint8_p)&packetBuffer, NET_HEADERSIZE, &readaddr);

            if (sequence != sock->receiveSequence) {
                receivedDuplicateCount++;
                continue;
            }
            sock->receiveSequence++;

            length -= NET_HEADERSIZE;

            if (flags & NETFLAG_EOM) {
                SZ_Clear(&net_message);
                SZ_Write(&net_message, sock->receiveMessage, sock->receiveMessageLength);
                SZ_Write(&net_message, packetBuffer.data, length);
                sock->receiveMessageLength = 0;

                ret = 1;
                break;
            }

            Q_memcpy(sock->receiveMessage + sock->receiveMessageLength, packetBuffer.data, length);
            sock->receiveMessageLength += length;
            continue;
        }
    }

    if (sock->sendNext)
        SendMessageNext(sock);

    return ret;
}


void PrintStats(qsocket_p s) {
    Con_Printf("canSend = %4u   \n", s->canSend);
    Con_Printf("sendSeq = %4u   ", s->sendSequence);
    Con_Printf("recvSeq = %4u   \n", s->receiveSequence);
    Con_Printf("\n");
}

void NET_Stats_f() {
    if (Cmd_Argc() == 1) {
        Con_Printf("unreliable messages sent   = %i\n", unreliableMessagesSent);
        Con_Printf("unreliable messages recv   = %i\n", unreliableMessagesReceived);
        Con_Printf("reliable messages sent     = %i\n", messagesSent);
        Con_Printf("reliable messages received = %i\n", messagesReceived);
        Con_Printf("packetsSent                = %i\n", packetsSent);
        Con_Printf("packetsReSent              = %i\n", packetsReSent);
        Con_Printf("packetsReceived            = %i\n", packetsReceived);
        Con_Printf("receivedDuplicateCount     = %i\n", receivedDuplicateCount);
        Con_Printf("shortPacketCount           = %i\n", shortPacketCount);
        Con_Printf("droppedDatagrams           = %i\n", droppedDatagrams);
    }
    else if (Q_strcmp(Cmd_Argv(1), "*") == 0) {
        for (qsocket_p s = net_activeSockets; s; s = s->next)
            PrintStats(s);
        for (qsocket_p s = net_freeSockets; s; s = s->next)
            PrintStats(s);
    }
    else {
        qsocket_p s = net_activeSockets;
        for (; s; s = s->next)
            if (Q_strcasecmp(Cmd_Argv(1), s->address) == 0)
                break;
        if (s == NULL)
            for (s = net_freeSockets; s; s = s->next)
                if (Q_strcasecmp(Cmd_Argv(1), s->address) == 0)
                    break;
        if (s == NULL)
            return;
        PrintStats(s);
    }
}


static bool _testInProgress = false;
static int _testPollCount;
static int _testDriver;
static int _testSocket;

static void Test_Poll();
PollProcedure testPollProcedure = {
    NULL,
    0.0,
    Test_Poll
};

static void Test_Poll() {
    qsockaddr_t clientaddr;
    char name[32];
    char address[64];

    _net_landriverlevel = _testDriver;

    while (1) {
        int len = dfunc.Read(_testSocket, net_message.data, net_message.maxsize, &clientaddr);
        if (len < sizeof(int))
            break;

        net_message.cursize = len;

        MSG_BeginReading();
        int control = BigLong(*((int*)net_message.data));
        MSG_ReadLong();
        if ((control == -1) ||
            ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL) ||
            ((control & NETFLAG_LENGTH_MASK) != len)
            ) {
            break;
        }

        if (MSG_ReadByte() != CCREP_PLAYER_INFO)
            Sys_Error("Unexpected repsonse to Player Info request\n");

        int playerNumber = MSG_ReadByte();
        Q_strcpy(name, MSG_ReadString());
        int colors = MSG_ReadLong();
        int frags = MSG_ReadLong();
        int connectTime = MSG_ReadLong();
        Q_strcpy(address, MSG_ReadString());

        Con_Printf(
            "%d:%s\n  frags:%3i  colors:%u %u  time:%u\n  %s\n",
            playerNumber,
            name, frags,
            colors >> 4, colors & 0x0f,
            connectTime / 60,
            address
        );
    }

    _testPollCount--;
    if (_testPollCount) {
        SchedulePollProcedure(&testPollProcedure, 0.1);
    }
    else {
        dfunc.CloseSocket(_testSocket);
        _testInProgress = false;
    }
}

static void Test_f() {
    int max = MAX_SCOREBOARD;
    qsockaddr_t sendaddr;

    if (_testInProgress)
        return;

    cString host = Cmd_Argv(1);

    if (host && hostCacheCount) {
        int n = 0;
        for (; n < hostCacheCount; n++)
            if (Q_strcasecmp(host, hostcache[n].name) == 0) {
                if (hostcache[n].driver != _myDriverLevel)
                    continue;
                _net_landriverlevel = hostcache[n].ldriver;
                max = hostcache[n].maxusers;
                Q_memcpy(&sendaddr, &hostcache[n].addr, sizeof(qsockaddr_t));
                break;
            }
        if (n < hostCacheCount)
            goto JustDoIt;
    }

    for (_net_landriverlevel = 0; _net_landriverlevel < net_numlandrivers; _net_landriverlevel++) {
        if (!net_landrivers[_net_landriverlevel].initialized)
            continue;

        // see if we can resolve the host name
        if (dfunc.GetAddrFromName(host, &sendaddr) != -1)
            break;
    }
    if (_net_landriverlevel == net_numlandrivers)
        return;

JustDoIt:
    _testSocket = dfunc.OpenSocket(0);
    if (_testSocket == -1)
        return;

    _testInProgress = true;
    _testPollCount = 20;
    _testDriver = _net_landriverlevel;

    for (int n = 0; n < max; n++) {
        SZ_Clear(&net_message);
        // save space for the header, filled in later
        MSG_WriteLong(&net_message, 0);
        MSG_WriteByte(&net_message, CCREQ_PLAYER_INFO);
        MSG_WriteByte(&net_message, n);
        *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
        dfunc.Write(_testSocket, net_message.data, net_message.cursize, &sendaddr);
    }
    SZ_Clear(&net_message);
    SchedulePollProcedure(&testPollProcedure, 0.1);
}


static bool _test2InProgress = false;
static int _test2Driver;
static int _test2Socket;

static void Test2_Poll();
PollProcedure test2PollProcedure = {
    NULL,
    0.0,
    Test2_Poll
};

static void Test2_Poll() {
    qsockaddr_t clientaddr;
    char name[256];
    char value[256];

    _net_landriverlevel = _test2Driver;
    name[0] = 0;

    int len = dfunc.Read(_test2Socket, net_message.data, net_message.maxsize, &clientaddr);
    if (len < sizeof(int))
        goto Reschedule;

    net_message.cursize = len;

    MSG_BeginReading();
    int control = BigLong(*((int*)net_message.data));
    MSG_ReadLong();
    if ((control == -1) ||
        ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL) ||
        ((control & NETFLAG_LENGTH_MASK) != len) ||
        (MSG_ReadByte() != CCREP_RULE_INFO))
        goto Error;

    Q_strcpy(name, MSG_ReadString());
    if (name[0] == 0)
        goto Done;
    Q_strcpy(value, MSG_ReadString());

    Con_Printf("%-16.16s  %-16.16s\n", name, value);

    SZ_Clear(&net_message);
    // save space for the header, filled in later
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
    MSG_WriteString(&net_message, name);
    *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
    dfunc.Write(_test2Socket, net_message.data, net_message.cursize, &clientaddr);
    SZ_Clear(&net_message);

Reschedule:
    SchedulePollProcedure(&test2PollProcedure, 0.05);
    return;

Error:
    Con_Printf("Unexpected repsonse to Rule Info request\n");
Done:
    dfunc.CloseSocket(_test2Socket);
    _test2InProgress = false;
    return;
}

static void Test2_f() {
    qsockaddr_t sendaddr;

    if (_test2InProgress)
        return;

    cString host = Cmd_Argv(1);

    if (host && hostCacheCount) {
        int n = 0;
        for (; n < hostCacheCount; n++)
            if (Q_strcasecmp(host, hostcache[n].name) == 0) {
                if (hostcache[n].driver != _myDriverLevel)
                    continue;
                _net_landriverlevel = hostcache[n].ldriver;
                Q_memcpy(&sendaddr, &hostcache[n].addr, sizeof(qsockaddr_t));
                break;
            }
        if (n < hostCacheCount)
            goto JustDoIt;
    }

    for (_net_landriverlevel = 0; _net_landriverlevel < net_numlandrivers; _net_landriverlevel++) {
        if (!net_landrivers[_net_landriverlevel].initialized)
            continue;

        // see if we can resolve the host name
        if (dfunc.GetAddrFromName(host, &sendaddr) != -1)
            break;
    }
    if (_net_landriverlevel == net_numlandrivers)
        return;

JustDoIt:
    _test2Socket = dfunc.OpenSocket(0);
    if (_test2Socket == -1)
        return;

    _test2InProgress = true;
    _test2Driver = _net_landriverlevel;

    SZ_Clear(&net_message);
    // save space for the header, filled in later
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
    MSG_WriteString(&net_message, "");
    *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
    dfunc.Write(_test2Socket, net_message.data, net_message.cursize, &sendaddr);
    SZ_Clear(&net_message);
    SchedulePollProcedure(&test2PollProcedure, 0.05);
}


int Datagram_Init() {
    _myDriverLevel = net_driverlevel;
    Cmd_AddCommand("net_stats", NET_Stats_f);

    if (COM_CheckParm("-nolan"))
        return -1;

    for (int i = 0; i < net_numlandrivers; i++) {
        int csock = net_landrivers[i].Init();
        if (csock == -1)
            continue;
        net_landrivers[i].initialized = true;
        net_landrivers[i].controlSock = csock;
    }

#ifdef BAN_TEST
    Cmd_AddCommand("ban", NET_Ban_f);
#endif
    Cmd_AddCommand("test", Test_f);
    Cmd_AddCommand("test2", Test2_f);

    return 0;
}


void Datagram_Shutdown() {
    //
    // shutdown the lan drivers
    //
    for (int32_t i = 0; i < net_numlandrivers; i++) {
        if (net_landrivers[i].initialized) {
            net_landrivers[i].Shutdown();
            net_landrivers[i].initialized = false;
        }
    }
}


void Datagram_Close(qsocket_p sock) {
    sfunc.CloseSocket(sock->socket);
}


void Datagram_Listen(bool state) {
    for (int32_t i = 0; i < net_numlandrivers; i++)
        if (net_landrivers[i].initialized)
            net_landrivers[i].Listen(state);
}


static qsocket_p _Datagram_CheckNewConnections() {
    qsockaddr_t clientaddr;
    qsockaddr_t newaddr;

    int acceptsock = dfunc.CheckNewConnections();
    if (acceptsock == -1)
        return NULL;

    SZ_Clear(&net_message);

    int len = dfunc.Read(acceptsock, net_message.data, net_message.maxsize, &clientaddr);
    if (len < sizeof(int))
        return NULL;
    net_message.cursize = len;

    MSG_BeginReading();
    int control = BigLong(*((int*)net_message.data));
    MSG_ReadLong();
    if (control == -1)
        return NULL;
    if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL)
        return NULL;
    if ((control & NETFLAG_LENGTH_MASK) != len)
        return NULL;

    ccreq_t command = MSG_ReadByte();
    if (command == CCREQ_SERVER_INFO) {
        if (Q_strcmp(MSG_ReadString(), "QUAKE") != 0)
            return NULL;

        SZ_Clear(&net_message);
        // save space for the header, filled in later
        MSG_WriteLong(&net_message, 0);
        MSG_WriteByte(&net_message, CCREP_SERVER_INFO);
        dfunc.GetSocketAddr(acceptsock, &newaddr);
        MSG_WriteString(&net_message, dfunc.AddrToString(&newaddr));
        MSG_WriteString(&net_message, hostname.string);
        MSG_WriteString(&net_message, sv.name);
        MSG_WriteByte(&net_message, net_activeconnections);
        MSG_WriteByte(&net_message, svs.maxclients);
        MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
        *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
        dfunc.Write(acceptsock, net_message.data, net_message.cursize, &clientaddr);
        SZ_Clear(&net_message);
        return NULL;
    }

    if (command == CCREQ_PLAYER_INFO) {
        int			playerNumber;
        int			activeNumber;
        int			clientNumber;
        client_t* client;

        playerNumber = MSG_ReadByte();
        activeNumber = -1;
        for (clientNumber = 0, client = svs.clients; clientNumber < svs.maxclients; clientNumber++, client++) {
            if (client->active) {
                activeNumber++;
                if (activeNumber == playerNumber)
                    break;
            }
        }
        if (clientNumber == svs.maxclients)
            return NULL;

        SZ_Clear(&net_message);
        // save space for the header, filled in later
        MSG_WriteLong(&net_message, 0);
        MSG_WriteByte(&net_message, CCREP_PLAYER_INFO);
        MSG_WriteByte(&net_message, playerNumber);
        MSG_WriteString(&net_message, client->name);
        MSG_WriteLong(&net_message, client->colors);
        MSG_WriteLong(&net_message, (int)client->edict->v.frags);
        MSG_WriteLong(&net_message, (int)(net_time - client->netconnection->connecttime));
        MSG_WriteString(&net_message, client->netconnection->address);
        *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
        dfunc.Write(acceptsock, net_message.data, net_message.cursize, &clientaddr);
        SZ_Clear(&net_message);

        return NULL;
    }

    if (command == CCREQ_RULE_INFO) {
        cString prevCvarName;
        cvar_p var;

        // find the search start location
        prevCvarName = MSG_ReadString();
        if (*prevCvarName) {
            var = Cvar_FindVar(prevCvarName);
            if (!var)
                return NULL;
            var = var->next;
        }
        else
            var = cvar_vars;

        // search for the next server cvar
        while (var) {
            // if (var->server)
            if (var->flags & cvf_server)
                break;
            var = var->next;
        }

        // send the response

        SZ_Clear(&net_message);
        // save space for the header, filled in later
        MSG_WriteLong(&net_message, 0);
        MSG_WriteByte(&net_message, CCREP_RULE_INFO);
        if (var) {
            MSG_WriteString(&net_message, var->name);
            MSG_WriteString(&net_message, var->string);
        }
        *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
        dfunc.Write(acceptsock, net_message.data, net_message.cursize, &clientaddr);
        SZ_Clear(&net_message);

        return NULL;
    }

    if ((command != CCREQ_CONNECT) ||
        (Q_strcmp(MSG_ReadString(), "QUAKE") != 0))
        return NULL;

    if (MSG_ReadByte() != NET_PROTOCOL_VERSION) {
        SZ_Clear(&net_message);
        // save space for the header, filled in later
        MSG_WriteLong(&net_message, 0);
        MSG_WriteByte(&net_message, CCREP_REJECT);
        MSG_WriteString(&net_message, "Incompatible version.\n");
        *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
        dfunc.Write(acceptsock, net_message.data, net_message.cursize, &clientaddr);
        SZ_Clear(&net_message);
        return NULL;
    }

#ifdef BAN_TEST
    // check for a ban
    if (clientaddr.sa_family == AF_INET) {
        uint32_t testAddr;
        testAddr = ((struct sockaddr_in*)&clientaddr)->sin_addr.s_addr;
        if ((testAddr & banMask) == banAddr) {
            SZ_Clear(&net_message);
            // save space for the header, filled in later
            MSG_WriteLong(&net_message, 0);
            MSG_WriteByte(&net_message, CCREP_REJECT);
            MSG_WriteString(&net_message, "You have been banned.\n");
            *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
            dfunc.Write(acceptsock, net_message.data, net_message.cursize, &clientaddr);
            SZ_Clear(&net_message);
            return NULL;
        }
    }
#endif

    // see if this guy is already connected
    for (qsocket_p s = net_activeSockets; s; s = s->next) {
        if (s->driver != net_driverlevel)
            continue;
        int ret = dfunc.AddrCompare(&clientaddr, &s->addr);
        if (ret >= 0) {
            // is this a duplicate connection reqeust?
            if (ret == 0 && net_time - s->connecttime < 2.0) {
                // yes, so send a duplicate reply
                SZ_Clear(&net_message);
                // save space for the header, filled in later
                MSG_WriteLong(&net_message, 0);
                MSG_WriteByte(&net_message, CCREP_ACCEPT);
                dfunc.GetSocketAddr(s->socket, &newaddr);
                MSG_WriteLong(&net_message, dfunc.GetSocketPort(&newaddr));
                *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
                dfunc.Write(acceptsock, net_message.data, net_message.cursize, &clientaddr);
                SZ_Clear(&net_message);
                return NULL;
            }
            // it's somebody coming back in from a crash/disconnect
            // so close the old qsocket and let their retry get them back in
            NET_Close(s);
            return NULL;
        }
    }

    // allocate a QSocket
    qsocket_p sock = NET_NewQSocket();
    if (sock == NULL) {
        // no room; try to let him know
        SZ_Clear(&net_message);
        // save space for the header, filled in later
        MSG_WriteLong(&net_message, 0);
        MSG_WriteByte(&net_message, CCREP_REJECT);
        MSG_WriteString(&net_message, "Server is full.\n");
        *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
        dfunc.Write(acceptsock, net_message.data, net_message.cursize, &clientaddr);
        SZ_Clear(&net_message);
        return NULL;
    }

    // allocate a network socket
    int newsock = dfunc.OpenSocket(0);
    if (newsock == -1) {
        NET_FreeQSocket(sock);
        return NULL;
    }

    // connect to the client
    if (dfunc.Connect(newsock, &clientaddr) == -1) {
        dfunc.CloseSocket(newsock);
        NET_FreeQSocket(sock);
        return NULL;
    }

    // everything is allocated, just fill in the details
    sock->socket = newsock;
    sock->landriver = _net_landriverlevel;
    sock->addr = clientaddr;
    Q_strcpy(sock->address, dfunc.AddrToString(&clientaddr));

    // send him back the info about the server connection he has been allocated
    SZ_Clear(&net_message);
    // save space for the header, filled in later
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREP_ACCEPT);
    dfunc.GetSocketAddr(newsock, &newaddr);
    MSG_WriteLong(&net_message, dfunc.GetSocketPort(&newaddr));
    //	MSG_WriteString(&net_message, dfunc.AddrToString(&newaddr));
    *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
    dfunc.Write(acceptsock, net_message.data, net_message.cursize, &clientaddr);
    SZ_Clear(&net_message);

    return sock;
}

qsocket_p Datagram_CheckNewConnections() {
    qsocket_p ret = NULL;

    for (_net_landriverlevel = 0; _net_landriverlevel < net_numlandrivers; _net_landriverlevel++)
        if ((net_landrivers[_net_landriverlevel].initialized) &&
            ((ret = _Datagram_CheckNewConnections()) != NULL))
            break;
    return ret;
}


static void _Datagram_SearchForHosts(bool xmit) {
    qsockaddr_t myaddr;
    dfunc.GetSocketAddr(dfunc.controlSock, &myaddr);
    if (xmit) {
        SZ_Clear(&net_message);
        // save space for the header, filled in later
        MSG_WriteLong(&net_message, 0);
        MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
        MSG_WriteString(&net_message, "QUAKE");
        MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
        *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
        dfunc.Broadcast(dfunc.controlSock, net_message.data, net_message.cursize);
        SZ_Clear(&net_message);
    }

    int ret;
    qsockaddr_t readaddr;
    while ((ret = dfunc.Read(dfunc.controlSock, net_message.data, net_message.maxsize, &readaddr)) > 0) {
        if (ret < sizeof(int))
            continue;
        net_message.cursize = ret;

        if ((dfunc.AddrCompare(&readaddr, &myaddr) >= 0) ||  // don't answer our own query
            (hostCacheCount == HOSTCACHESIZE))    // is the cache full?
            continue;

        MSG_BeginReading();
        int control = BigLong(*((int*)net_message.data));
        MSG_ReadLong();
        if ((control == -1) ||
            ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL) ||
            ((control & NETFLAG_LENGTH_MASK) != ret) ||
            (MSG_ReadByte() != CCREP_SERVER_INFO))
            continue;

        dfunc.GetAddrFromName(MSG_ReadString(), &readaddr);
        // search the cache for this server
        int n = 0;
        for (; n < hostCacheCount; n++)
            if (dfunc.AddrCompare(&readaddr, &hostcache[n].addr) == 0)
                break;

        // is it already there?
        if (n < hostCacheCount)
            continue;

        // add it
        hostCacheCount++;
        Q_strcpy(hostcache[n].name, MSG_ReadString());
        Q_strcpy(hostcache[n].map, MSG_ReadString());
        hostcache[n].users = MSG_ReadByte();
        hostcache[n].maxusers = MSG_ReadByte();
        if (MSG_ReadByte() != NET_PROTOCOL_VERSION) {
            Q_strcpy(hostcache[n].cname, hostcache[n].name);
            hostcache[n].cname[14] = 0;
            Q_strcpy(hostcache[n].name, "*");
            Q_strcat(hostcache[n].name, hostcache[n].cname);
        }
        Q_memcpy(&hostcache[n].addr, &readaddr, sizeof(qsockaddr_t));
        hostcache[n].driver = net_driverlevel;
        hostcache[n].ldriver = _net_landriverlevel;
        Q_strcpy(hostcache[n].cname, dfunc.AddrToString(&readaddr));

        // check for a name conflict
        for (int i = 0; i < hostCacheCount; i++) {
            if (i == n)
                continue;
            if (Q_strcasecmp(hostcache[n].name, hostcache[i].name) == 0) {
                i = Q_strlen(hostcache[n].name);
                if (i < 15 && hostcache[n].name[i - 1] > '8') {
                    hostcache[n].name[i] = '0';
                    hostcache[n].name[i + 1] = 0;
                }
                else
                    hostcache[n].name[i - 1]++;
                i = -1;
            }
        }
    }
}

void Datagram_SearchForHosts(bool xmit) {
    for (_net_landriverlevel = 0; _net_landriverlevel < net_numlandrivers; _net_landriverlevel++) {
        if (hostCacheCount == HOSTCACHESIZE)
            break;
        if (net_landrivers[_net_landriverlevel].initialized)
            _Datagram_SearchForHosts(xmit);
    }
}


static qsocket_p _Datagram_Connect(cString host) {
    qsockaddr_t sendaddr;
    qsockaddr_t readaddr;

    // see if we can resolve the host name
    if (dfunc.GetAddrFromName(host, &sendaddr) == -1)
        return NULL;

    int newsock = dfunc.OpenSocket(0);
    if (newsock == -1)
        return NULL;

    qsocket_p sock = NET_NewQSocket();
    if (sock == NULL)
        goto ErrorReturn2;
    sock->socket = newsock;
    sock->landriver = _net_landriverlevel;

    // connect to the host
    if (dfunc.Connect(newsock, &sendaddr) == -1)
        goto ErrorReturn;

    // send the connection request
    Con_Printf("trying...\n"); SCR_UpdateScreen();
    double start_time = net_time;

    int ret;
    for (int reps = 0; reps < 3; reps++) {
        SZ_Clear(&net_message);
        // save space for the header, filled in later
        MSG_WriteLong(&net_message, 0);
        MSG_WriteByte(&net_message, CCREQ_CONNECT);
        MSG_WriteString(&net_message, "QUAKE");
        MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
        *((int*)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
        dfunc.Write(newsock, net_message.data, net_message.cursize, &sendaddr);
        SZ_Clear(&net_message);
        do {
            ret = dfunc.Read(newsock, net_message.data, net_message.maxsize, &readaddr);
            // if we got something, validate it
            if (ret > 0) {
                // is it from the right place?
                if (sfunc.AddrCompare(&readaddr, &sendaddr) != 0) {
#ifdef DEBUG
                    Con_Printf("wrong reply address\n");
                    Con_Printf("Expected: %s\n", StrAddr(&sendaddr));
                    Con_Printf("Received: %s\n", StrAddr(&readaddr));
                    SCR_UpdateScreen();
#endif
                    ret = 0;
                    continue;
                }

                if (ret < sizeof(int)) {
                    ret = 0;
                    continue;
                }

                net_message.cursize = ret;
                MSG_BeginReading();

                int control = BigLong(*((int*)net_message.data));
                MSG_ReadLong();
                if (control == -1) {
                    ret = 0;
                    continue;
                }
                if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL) {
                    ret = 0;
                    continue;
                }
                if ((control & NETFLAG_LENGTH_MASK) != ret) {
                    ret = 0;
                    continue;
                }
            }
        } while (ret == 0 && (SetNetTime() - start_time) < 2.5);
        if (ret)
            break;
        Con_Printf("still trying...\n"); SCR_UpdateScreen();
        start_time = SetNetTime();
    }

    if (ret == 0) {
        cString reason = "No Response";
        Con_Printf("%s\n", reason);
        Q_strcpy(m_return_reason, reason);
        goto ErrorReturn;
    }

    if (ret == -1) {
        cString reason = "Network Error";
        Con_Printf("%s\n", reason);
        Q_strcpy(m_return_reason, reason);
        goto ErrorReturn;
    }

    ret = MSG_ReadByte();
    if (ret == CCREP_REJECT) {
        cString reason = MSG_ReadString();
        Con_Printf(reason);
        Q_strncpy(m_return_reason, reason, 31);
        goto ErrorReturn;
    }

    if (ret == CCREP_ACCEPT) {
        Q_memcpy(&sock->addr, &sendaddr, sizeof(qsockaddr_t));
        dfunc.SetSocketPort(&sock->addr, MSG_ReadLong());
    }
    else {
        cString reason = "Bad Response";
        Con_Printf("%s\n", reason);
        Q_strcpy(m_return_reason, reason);
        goto ErrorReturn;
    }

    dfunc.GetNameFromAddr(&sendaddr, sock->address);

    Con_Printf("Connection accepted\n");
    sock->lastMessageTime = SetNetTime();

    // switch the connection to the specified address
    if (dfunc.Connect(newsock, &sock->addr) == -1) {
        cString reason = "Connect to Game failed";
        Con_Printf("%s\n", reason);
        Q_strcpy(m_return_reason, reason);
        goto ErrorReturn;
    }

    m_return_onerror = false;
    return sock;

ErrorReturn:
    NET_FreeQSocket(sock);
ErrorReturn2:
    dfunc.CloseSocket(newsock);
    if (m_return_onerror) {
        key_dest = key_menu;
        m_state = m_return_state;
        m_return_onerror = false;
    }
    return NULL;
}

qsocket_p Datagram_Connect(cString host) {
    qsocket_p ret = NULL;

    for (_net_landriverlevel = 0; _net_landriverlevel < net_numlandrivers; _net_landriverlevel++)
        if ((net_landrivers[_net_landriverlevel].initialized) &&
            ((ret = _Datagram_Connect(host)) != NULL))
            break;
    return ret;
}
