#pragma once
#include "types.h"
#include "qSocket.h"

/* Transport layer: raw bytes, OS sockets, addresses, ports. No Quake session knowledge. */
struct NetLanDriver_t {
    cString  name;
    bool     initialized = false;
    int32_t  controlSock = 0;

    /* mandatory */
    virtual int      Init()                                                         = 0;
    virtual void     Shutdown()                                                     = 0;
    virtual int      OpenSocket(int32_t port)                                       = 0;
    virtual int      CloseSocket(int socket)                                        = 0;
    virtual int32_t  Connect(int socket, qsockaddr_p addr)                          = 0;
    virtual int32_t  CheckNewConnections()                                          = 0;
    virtual int32_t  Read(int socket, uint8_p buf, int32_t len, qsockaddr_p addr)   = 0;
    virtual int32_t  Write(int socket, uint8_p buf, int32_t len, qsockaddr_p addr)  = 0;
    virtual cString  AddrToString(qsockaddr_p addr)                                 = 0;
    virtual int32_t  StringToAddr(cString string, qsockaddr_p addr)                 = 0;
    virtual int32_t  GetSocketAddr(int socket, qsockaddr_p addr)                    = 0;
    virtual int32_t  GetAddrFromName(cString name, qsockaddr_p addr)                = 0;
    virtual int32_t  AddrCompare(qsockaddr_p addr1, qsockaddr_p addr2)              = 0;
    virtual int32_t  GetSocketPort(qsockaddr_p addr)                                = 0;
    virtual int32_t  SetSocketPort(qsockaddr_p addr, int32_t port)                  = 0;

    /* optional */
    virtual void     Listen(bool state)                                 {}
    virtual int32_t  Broadcast(int socket, uint8_p buf, int32_t len)    { return 0; }
    virtual int32_t  GetNameFromAddr(qsockaddr_p addr, cString name)    { return 0; }

    virtual ~NetLanDriver_t() = default;
};


#include "sizebuf.h"
/* Protocol layer: Quake sessions, reliable/unreliable messages, connection state.
 * Datagram impl delegates to NetLanDriver_t; VCR bypasses it entirely. */
struct NetDriver_t {
    cString  name;
    bool     initialized = false;
    int32_t  controlSock = 0;

    /* mandatory */
    virtual int        Init()                                       = 0;
    virtual qsocket_p  Connect(cString host)                        = 0;
    virtual qsocket_p  CheckNewConnections()                        = 0;
    virtual int        QGetMessage(qsocket_p sock)                  = 0;
    virtual int        QSendMessage(qsocket_p sock, sizebuf_p data) = 0;
    virtual bool       CanSendMessage(qsocket_p sock)               = 0;
    virtual void       Close(qsocket_p sock)                        = 0;
    virtual void       Shutdown()                                   = 0;
    virtual void       SearchForHosts(bool xmit)                    = 0;

    /* optional */
    virtual void  Listen(bool state)                               {}
    virtual int   SendUnreliableMessage(qsocket_p, sizebuf_p)      { return 0; }
    virtual bool  CanSendUnreliableMessage(qsocket_p)              { return false; }

    virtual ~NetDriver_t() = default;
};