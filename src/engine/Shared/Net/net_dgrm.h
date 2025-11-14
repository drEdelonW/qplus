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
// net_dgrm.h
#include "net.h"

int Datagram_Init();
void Datagram_Listen(bool state);
void Datagram_SearchForHosts(bool xmit);
qsocket_p Datagram_Connect(cString host);
qsocket_p Datagram_CheckNewConnections();
int Datagram_GetMessage(qsocket_p sock);
int Datagram_SendMessage(qsocket_p sock, sizebuf_p data);
int Datagram_SendUnreliableMessage(qsocket_p sock, sizebuf_p data);
bool Datagram_CanSendMessage(qsocket_p sock);
bool Datagram_CanSendUnreliableMessage(qsocket_p sock);
void Datagram_Close(qsocket_p sock);
void Datagram_Shutdown();
