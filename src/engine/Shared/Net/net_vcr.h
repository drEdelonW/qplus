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
// net_vcr.h
#include "net.h"

typedef enum {
    VCR_OP_CONNECT       = 1u,
    VCR_OP_GETMESSAGE    = 2u,
    VCR_OP_SENDMESSAGE   = 3u,
    VCR_OP_CANSENDMESSAGE= 4u,

    VCR_MAX_MESSAGE      = 4u  // highest valid opcode
} vcr_opcode_t;

extern int  vcrFile;
#define VCR_SIGNATURE (uint32_t)(0x56435231)

int VCR_Init();
void VCR_Listen(bool state);
void VCR_SearchForHosts(bool xmit);
qsocket_p VCR_Connect(cString host);
qsocket_p VCR_CheckNewConnections();
int VCR_GetMessage(qsocket_p sock);
int VCR_SendMessage(qsocket_p sock, sizebuf_p data);
bool VCR_CanSendMessage(qsocket_p sock);
void VCR_Close(qsocket_p sock);
void VCR_Shutdown();
