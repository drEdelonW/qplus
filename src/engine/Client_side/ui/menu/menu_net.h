#pragma once

typedef struct {
    int  port;
    char portname[6];
    char joinname[22];
} LanConfig_t;
extern LanConfig_t lanConfig;

extern int m_net_cursor;
#define SerialConfig    (m_net_cursor == 0)
#define DirectConfig    (m_net_cursor == 1)
#define IPXConfig       (m_net_cursor == 2)
#define TCPIPConfig     (m_net_cursor == 3)

bool is_CreateGame();
bool is_JoinGame();