#pragma once
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
// net.h -- quake's interface to the networking layer
#include "types.h"
#include "sizebuf.h"

#define MAX_MSGLEN  8000  // max length of a reliable message
#define MAX_DATAGRAM 1024  // max length of unreliable message

typedef struct qsockaddr {
    int16_t sa_family;
    uint8_t sa_data[14];
} qsockaddr_t;
typedef qsockaddr_t* qsockaddr_p;


#define NET_NAMELEN   64

#define NET_MAXMESSAGE  8192
#define NET_HEADERSIZE  (2 * sizeof(uint32_t))
#define NET_DATAGRAMSIZE (MAX_DATAGRAM + NET_HEADERSIZE)

// NetHeader flags
typedef enum {
    NETFLAG_LENGTH_MASK = 0x0000ffff, // lower 16 bits hold length
    NETFLAG_DATA        = 0x00010000, // packet contains data
    NETFLAG_ACK         = 0x00020000, // acknowledge
    NETFLAG_NAK         = 0x00040000, // negative acknowledge
    NETFLAG_EOM         = 0x00080000, // end of message
    NETFLAG_UNRELIABLE  = 0x00100000, // unreliable packet
    NETFLAG_CTL         = 0x80000000  // control packet
} netflag_t;


#define NET_PROTOCOL_VERSION 3

// This is the network info/connection protocol.  It is used to find Quake
// servers, get info about them, and connect to them.  Once connected, the
// Quake game protocol (documented elsewhere) is used.
//
//
// General notes:
// game_name is currently always "QUAKE", but is there so this same protocol
// can be used for future games as well; can you say Quake2?
//
// CCREQ_CONNECT
//      string  game_name               "QUAKE"
//      byte    net_protocol_version    NET_PROTOCOL_VERSION
//
// CCREQ_SERVER_INFO
//      string  game_name               "QUAKE"
//      byte    net_protocol_version    NET_PROTOCOL_VERSION
//
// CCREQ_PLAYER_INFO
//      byte    player_number
//
// CCREQ_RULE_INFO
//      string  rule
//
//
//
// CCREP_ACCEPT
//      long    port
//
// CCREP_REJECT
//      string  reason
//
// CCREP_SERVER_INFO
//      string  server_address
//      string  host_name
//      string  level_name
//      byte    current_players
//      byte    max_players
//      byte    protocol_version    NET_PROTOCOL_VERSION
//
// CCREP_PLAYER_INFO
//      byte    player_number
//      string  name
//      long    colors
//      long    frags
//      long    connect_time
//      string  address
//
// CCREP_RULE_INFO
//      string  rule
//      string  value

//  note:
//      There are two address forms used above.  The int16_t form is just a
//      port number.  The address that goes along with the port is defined as
//      "whatever address you receive this reponse from".  This lets us use
//      the host OS to solve the problem of multiple host addresses (possibly
//      with no routing between them); the host will use the right address
//      when we reply to the inbound connection request.  The long from is
//      a full address and port in a string.  It is used for returning the
//      address of a server that is not running locally.

typedef enum {
    // requests
    CCREQ_CONNECT       = 0x01,
    CCREQ_SERVER_INFO   = 0x02,
    CCREQ_PLAYER_INFO   = 0x03,
    CCREQ_RULE_INFO     = 0x04,

    // responses
    CCREP_ACCEPT        = 0x81,
    CCREP_REJECT        = 0x82,
    CCREP_SERVER_INFO   = 0x83,
    CCREP_PLAYER_INFO   = 0x84,
    CCREP_RULE_INFO     = 0x85
} ccreq_t;

typedef struct qsocket_s qsocket_t;
typedef qsocket_t* qsocket_p;
struct qsocket_s {
    qsocket_p next;
    double connecttime;
    double lastMessageTime;
    double lastSendTime;

    bool disconnected;
    bool canSend;
    bool sendNext;

    int32_t driver;
    int32_t landriver;
    int socket;
    TypeLess_ptr driverdata;

    uint32_t ackSequence;
    uint32_t sendSequence;
    uint32_t unreliableSendSequence;
    int32_t sendMessageLength;
    uint8_t sendMessage[NET_MAXMESSAGE];

    uint32_t receiveSequence;
    uint32_t unreliableReceiveSequence;
    int32_t receiveMessageLength;
    uint8_t receiveMessage[NET_MAXMESSAGE];

    qsockaddr_t addr;
    char address[NET_NAMELEN];
};

extern qsocket_p net_activeSockets;
extern qsocket_p net_freeSockets;
extern int32_t   net_numsockets;

typedef struct {
    cString name;
    bool initialized;
    int32_t controlSock;
    int (*Init)();
    void (*Shutdown)();
    void (*Listen)(bool state);
    int (*OpenSocket)(int32_t port);
    int (*CloseSocket)(int  socket);
    int32_t(*Connect)(int socket, qsockaddr_p addr);
    int32_t(*CheckNewConnections)();
    int32_t(*Read)(int socket, uint8_p buf, int32_t len, qsockaddr_p addr);
    int32_t(*Write)(int socket, uint8_p buf, int32_t len, qsockaddr_p addr);
    int32_t(*Broadcast)(int socket, uint8_p buf, int32_t len);
    cString(*AddrToString)(qsockaddr_p addr);
    int32_t(*StringToAddr)(cString string, qsockaddr_p addr);
    int32_t(*GetSocketAddr)(int socket, qsockaddr_p addr);
    int32_t(*GetNameFromAddr)(qsockaddr_p addr, cString name);
    int32_t(*GetAddrFromName)(cString name, qsockaddr_p addr);
    int32_t(*AddrCompare)(qsockaddr_p addr1, qsockaddr_p addr2);
    int32_t(*GetSocketPort)(qsockaddr_p addr);
    int32_t(*SetSocketPort)(qsockaddr_p addr, int32_t port);
} net_landriver_t;

#define MAX_NET_DRIVERS  8
extern int32_t net_numlandrivers;
extern net_landriver_t net_landrivers[MAX_NET_DRIVERS];

typedef struct {
    cString name;
    bool initialized;
    int (*Init)();
    void (*Listen)(bool state);
    void (*SearchForHosts)(bool xmit);
    qsocket_p(*Connect)(cString host);
    qsocket_p(*CheckNewConnections)();
    int(*QGetMessage)(qsocket_p sock);
    int(*QSendMessage)(qsocket_p sock, sizebuf_p data);
    int(*SendUnreliableMessage)(qsocket_p sock, sizebuf_p data);
    bool (*CanSendMessage)(qsocket_p sock);
    bool (*CanSendUnreliableMessage)(qsocket_p sock);
    void (*Close)(qsocket_p sock);
    void (*Shutdown)();
    int32_t controlSock;
} net_driver_t;

extern bool recording;
extern int32_t net_numdrivers;
extern net_driver_t net_drivers[MAX_NET_DRIVERS];

extern int32_t DEFAULTnet_hostport;
extern int32_t net_hostport;

extern int32_t net_driverlevel;

extern char playername[];
extern int32_t playercolor;

extern int32_t messagesSent;
extern int32_t messagesReceived;
extern int32_t unreliableMessagesSent;
extern int32_t unreliableMessagesReceived;

extern bool serialAvailable;
extern bool ipxAvailable;
extern bool tcpipAvailable;

extern char my_ipx_address[NET_NAMELEN];
extern char my_tcpip_address[NET_NAMELEN];
#define HOSTCACHESIZE 8

typedef struct {
    char name[16];
    char map[16];
    char cname[32];
    int32_t users;
    int32_t maxusers;
    int32_t driver;
    int32_t ldriver;
    qsockaddr_t addr;
} hostcache_t;

extern int32_t hostCacheCount;
extern hostcache_t hostcache[HOSTCACHESIZE];

extern double       net_time;
extern sizebuf_t    net_message;
extern int32_t      net_activeconnections;
typedef struct _PollProcedure {
    struct _PollProcedure* next;
    double nextTime;
    void (*procedure)();
    TypeLess_ptr arg;
} PollProcedure;

extern bool slistInProgress;
extern bool slistSilent;
extern bool slistLocal;

#ifdef __cplusplus
extern "C" {
#endif
    qsocket_p NET_NewQSocket();
    void NET_FreeQSocket(qsocket_p);
    double SetNetTime();

#if !defined(_WIN32 ) && !defined (__linux__) && !defined (__sun__)
#ifndef htonl
    extern uint32_t htonl(uint32_t hostlong);
#endif
#ifndef htons
    extern uint16_t htons(uint16_t hostshort);
#endif
#ifndef ntohl
    extern uint32_t ntohl(uint32_t netlong);
#endif
#ifndef ntohs
    extern uint16_t ntohs(uint16_t netshort);
#endif
#endif

#ifdef IDGODS
    bool IsID(qsockaddr_p addr);
#endif

    //============================================================================
    //
    // public network functions
    //
    //============================================================================


    void NET_Init();
    void NET_Shutdown();

    qsocket_p NET_CheckNewConnections();    // returns a new connection number if there is one pending, else -1
    qsocket_p NET_Connect(cString host);    // called by client to connect to a host.  Returns -1 if not able to
    bool NET_CanSendMessage(qsocket_p sock);   // Returns true or false if the given qsocket can currently accept a message to be transmitted.

    int32_t NET_GetMessage(qsocket_p sock);
    // returns data in net_message sizebuf
    // returns 0 if no data is waiting
    // returns 1 if a message was received
    // returns 2 if an unreliable message was received
    // returns -1 if the connection died

    int32_t NET_SendMessage(qsocket_p sock, sizebuf_p data);
    int32_t NET_SendUnreliableMessage(qsocket_p sock, sizebuf_p data);
    // returns 0 if the message connot be delivered reliably, but the connection is still considered valid
    // returns 1 if the message was sent properly
    // returns -1 if the connection died

    int32_t NET_SendToAll(sizebuf_p data, int32_t blocktime);    // This is a reliable *blocking* send to all attached clients.
    void NET_Close(qsocket_p sock);  // if a dead connection is returned by a get or send function, this function should be called when it is convenient

    // Server calls when a client is kicked off for a game related misbehavior
    // like an illegal protocal conversation.  Client calls when disconnecting
    // from a server.
    // A netcon_t number will not be reused until this function is called for it

    void NET_Poll();

    void SchedulePollProcedure(PollProcedure* pp, double timeOffset);


    extern void (*GetComPortConfig)(int32_t portNumber, int32_p port, int32_p irq, int32_p baud, bool* useModem);
    extern void (*SetComPortConfig)(int32_t portNumber, int32_t port, int32_t irq, int32_t baud, bool useModem);
    extern void (*GetModemConfig)(int32_t portNumber, cString dialType, cString clear, cString init, cString hangup);
    extern void (*SetModemConfig)(int32_t portNumber, cString dialType, cString clear, cString init, cString hangup);

    void NET_Slist_f();

#ifdef __cplusplus
}
#endif