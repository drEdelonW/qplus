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
#include <string.h>
#include "console.h"
#include "host.h"
#include "Edict.h"
#include "server.h"
#include "GameRule.h"
#include "msg.h"
#include "protocol.h"


cString PF_VarString(int first) {
    static char _out[256];

    _out[0] = 0;
    for (int i = first; i < pr_argc; i++)
        strcat(_out, G_STRING((OFS_PARM0 + i * 3)));

    return _out;
}


/*
=================
PF_errror

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

error(value)
=================
*/

void PF_error() {
    cString str = PF_VarString(0);
    Con_Printf(
        "======SERVER ERROR in %s:\n%s\n",
        PR_GetQString(pr_xFunction->s_name),
        str);
    edict_p ed = ED_GetEDictByOffs(pr_global_struct->self);
    ED_Print(ed);

    Host_Error("Program error");
}


/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
void PF_objerror() {
    cString str = PF_VarString(0);
    Con_Printf(
        "======OBJECT ERROR in %s:\n%s\n",
        PR_GetQString(pr_xFunction->s_name),
        str);
    edict_p ed = ED_GetEDictByOffs(pr_global_struct->self);
    ED_Print(ed);
    ED_Free(ed);

    Host_Error("Program error");
}


/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void PF_bprint() {
    cString str = PF_VarString(0);
    SV_BroadcastPrintf("%s", str);
}


/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void PF_sprint() {
    uint32_t ent_num = G_EDICTNUM(OFS_PARM0);
    cString str = PF_VarString(1);

    if ((ent_num < 1) ||
        (ent_num > GetSvMaxClients())) {
        Con_Printf("tried to sprint to a non-client\n");    return;
    }

    RmtClient_p client = &svs.clients[ent_num - 1];

    MSG_WriteChar(&client->message, svc_print);
    MSG_WriteString(&client->message, str);
}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void PF_centerprint() {
    uint32_t entnum = G_EDICTNUM(OFS_PARM0);
    cString str = PF_VarString(1);

    if ((entnum < 1) || (entnum > GetSvMaxClients())) {
        Con_Printf("tried to sprint to a non-client\n");    return;
    }

    RmtClient_p client = &svs.clients[entnum - 1];

    MSG_WriteChar(&client->message, svc_centerprint);
    MSG_WriteString(&client->message, str);
}


/*
=========
PF_dprint
=========
*/
void PF_dprint() { Con_DPrintf("%s", PF_VarString(0)); }

static char _pr_string_temp[128];

void PF_ftos() {
    float v = G_FLOAT(OFS_PARM0);

    if (v == (int)v)    snprintf(_pr_string_temp, sizeof(_pr_string_temp), "%d", (int)v);
    else                snprintf(_pr_string_temp, sizeof(_pr_string_temp), "%5.1f", v);
    G_INT(OFS_RETURN) = PR_SetQString(_pr_string_temp);
}



void PF_vtos() {
    snprintf(
        _pr_string_temp, sizeof(_pr_string_temp),
        "'%5.1f %5.1f %5.1f'",
        G_VECTOR(OFS_PARM0).x,
        G_VECTOR(OFS_PARM0).y,
        G_VECTOR(OFS_PARM0).z
    );
    G_INT(OFS_RETURN) = PR_SetQString(_pr_string_temp);
}


#ifdef QUAKE2
void PF_etos() {
    snprintf(_pr_string_temp, sizeof(_pr_string_temp), "entity %i", G_EDICTNUM(OFS_PARM0));
    G_INT(OFS_RETURN) = PR_SetQString(_pr_string_temp);
}
#endif



/*
=================
PF_break

break()
=================
*/
void PF_break() {
    Con_Printf("break statement\n");
    *(int*)-4 = 0; // dump to debugger
    // PR_RunError ("break statement");
}

