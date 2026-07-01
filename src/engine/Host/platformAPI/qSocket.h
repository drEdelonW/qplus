#pragma once
#include "qTime.h"

#define NET_MAXMESSAGE  8192
#define NET_NAMELEN   64


typedef struct qsockaddr {
    int16_t sa_family;
    uint8_t sa_data[14];
} qsockaddr_t;
typedef qsockaddr_t* qsockaddr_p;

typedef struct qsocket_s qsocket_t;
typedef qsocket_t* qsocket_p;
struct qsocket_s {
    qsocket_p next;
    LegTime_t connecttime;
    LegTime_t lastMessageTime;
    LegTime_t lastSendTime;

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