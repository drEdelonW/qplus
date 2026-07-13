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
#include "GlobVars.h"
#include "server.h"
#include "GameRule.h"
#include "host.h"
#include "cvar.h"
#include "cbuf.h"

/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF_stuffcmd() {
    EdIdx entnum = G_EDICTNUM(OFS_PARM0);
    if ((entnum < EdictWorld) ||
        (entnum > GetSvMaxClients())
        )   PR_RunError("Parm 0 not a client");
    else {
        RmtClient_p old = remoteClient;
        remoteClient = &svs.clients[entnum - 1];
        Host_ClientCommands("%s", G_STRING(OFS_PARM1));
        remoteClient = old;
    }
}


void PF_localcmd() { Cbuf_AddText(G_STRING(OFS_PARM0)); }
void PF_cvar() { G_FLOAT(OFS_RETURN) = Cvar_VariableValue(G_STRING(OFS_PARM0)); }
void PF_cvar_set() { Cvar_Set(/*var*/G_STRING(OFS_PARM0), /*val*/G_STRING(OFS_PARM1)); }
