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
// net_wins.h
#include <stdint.h>
typedef uint8_t byte;

int  WINS_Init(void);
void WINS_Shutdown(void);
void WINS_Listen(bool state);
int  WINS_OpenSocket(int port);
int  WINS_CloseSocket(int socket);
int  WINS_Connect(int socket, qsockaddr_p addr);
int  WINS_CheckNewConnections(void);
int  WINS_Read(int socket, byte* buf, int len, qsockaddr_p addr);
int  WINS_Write(int socket, byte* buf, int len, qsockaddr_p addr);
int  WINS_Broadcast(int socket, byte* buf, int len);
cString WINS_AddrToString(qsockaddr_p addr);
int  WINS_StringToAddr(cString string, qsockaddr_p addr);
int  WINS_GetSocketAddr(int socket, qsockaddr_p addr);
int  WINS_GetNameFromAddr(qsockaddr_p addr, cString name);
int  WINS_GetAddrFromName(cString name, qsockaddr_p addr);
int  WINS_AddrCompare(qsockaddr_p addr1, qsockaddr_p addr2);
int  WINS_GetSocketPort(qsockaddr_p addr);
int  WINS_SetSocketPort(qsockaddr_p addr, int port);
