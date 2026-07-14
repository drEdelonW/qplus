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

#include "progs.h"
#include "progdefs.h"
#include "GlobVars.h"
#include "sizebuf.h"
#include "server.h"
#include "msg.h"
#include "GameRule.h"


/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

typedef enum msg_dest_e {
    MSG_BROADCAST = 0u,  // unreliable to all
    MSG_ONE = 1u,  // reliable to one (msg_entity)
    MSG_ALL = 2u,  // reliable to all
    MSG_INIT = 3u   // write to the init string
} msg_dest_e;

sizebuf_p WriteDest() {
    switch ((msg_dest_e)G_FLOAT(OFS_PARM0)) {
    case MSG_BROADCAST: return &sv.datagram;
    case MSG_ALL:       return &sv.reliable_datagram;
    case MSG_INIT:      return &sv.signon;
    case MSG_ONE: {
        uint32_t entnum = ED_GetEDictIdx(ED_GetEDictByOffs(GV_pGame()->msg_entity));
        if ((entnum < 1) ||
            (entnum > GetSvMaxClients())
            )   PR_RunError("WriteDest: not a client");
        return &svs.clients[entnum - 1].message;
    }
    default:    PR_RunError("WriteDest: bad destination");  break;
    }

    return NULL;
}

void PF_WriteByte() { MSG_WriteByte(WriteDest(), (uint8_t)G_FLOAT(OFS_PARM1)); }
void PF_WriteChar() { MSG_WriteChar(WriteDest(), (int8_t)G_FLOAT(OFS_PARM1)); }
void PF_WriteShort() { MSG_WriteShort(WriteDest(), (int16_t)G_FLOAT(OFS_PARM1)); }
void PF_WriteLong() { MSG_WriteLong(WriteDest(), (int32_t)G_FLOAT(OFS_PARM1)); }
void PF_WriteAngle() { MSG_WriteAngle(WriteDest(), G_FLOAT(OFS_PARM1)); }
void PF_WriteCoord() { MSG_WriteCoord(WriteDest(), G_FLOAT(OFS_PARM1)); }
void PF_WriteString() { MSG_WriteString(WriteDest(), G_STRING(OFS_PARM1)); }
void PF_WriteEntity() { MSG_WriteShort(WriteDest(), (int16_t)G_EDICTNUM(OFS_PARM1)); }
