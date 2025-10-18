#pragma once

extern int lanConfig_port;

extern int m_net_cursor;
#define SerialConfig    (m_net_cursor == 0)
#define DirectConfig    (m_net_cursor == 1)
#define IPXConfig       (m_net_cursor == 2)
#define TCPIPConfig     (m_net_cursor == 3)

bool is_CreateGame();
bool is_JoinGame();