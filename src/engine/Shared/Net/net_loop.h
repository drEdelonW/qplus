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
// net_loop.h
#include "net.h"

int Loop_Init();
void Loop_Listen(bool state);
void Loop_SearchForHosts(bool xmit);
qsocket_p Loop_Connect(cString host);
qsocket_p Loop_CheckNewConnections();
NetGetMessageResult Loop_GetMessage(qsocket_p sock);
int Loop_SendMessage(qsocket_p sock, sizebuf_p data);
int Loop_SendUnreliableMessage(qsocket_p sock, sizebuf_p data);
bool Loop_CanSendMessage(qsocket_p sock);
bool Loop_CanSendUnreliableMessage(qsocket_p sock);
void Loop_Close(qsocket_p sock);
void Loop_Shutdown();
