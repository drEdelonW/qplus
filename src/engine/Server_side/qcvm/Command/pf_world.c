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
#include "world.h"
#include "server.h"
#include "server_priv.h"
#include "GameRule.h"
#include "msg.h"
#include "VA.h"
#include "protocol.h"
#include "cbuf.h"
#include <string.h>

/*
=============
PF_pointcontents
=============
*/
void PF_pointcontents() {
    vec3_t v = G_VECTOR(OFS_PARM0);
    G_FLOAT(OFS_RETURN) = SV_PointContents(v);
}


/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void PF_lightstyle() {
    int style = (int)G_FLOAT(OFS_PARM0);
    cString val = G_STRING(OFS_PARM1);

    // change the string in sv
    sv.lightstyles[style] = val;

    // send message to all clients on this server
    if (sv.state != ss_active) return;

    RmtClient_p client = svs.clients;
    for (int j = 0; j < GetSvMaxClients(); j++, client++)
        if (client->active || client->spawned) {
            MSG_WriteChar(&client->message, svc_lightstyle);
            MSG_WriteChar(&client->message, (int8_t)style);
            MSG_WriteString(&client->message, val);
        }
}


/*
=================
PF_particle

particle(origin, color, count)
=================
*/
void PF_particle() {
    vec3_t org = G_VECTOR(OFS_PARM0);
    vec3_t dir = G_VECTOR(OFS_PARM1);
    float color = G_FLOAT(OFS_PARM2);
    float count = G_FLOAT(OFS_PARM3);
    SV_StartParticle(org, dir, (int)color, (size_t)count);
}

void PR_CheckEmptyString(cString str) {
    if (str[0] <= ' ')
        PR_RunError("Bad string");
}


void PF_precache_sound() {
    if (sv.state != ss_loading)
        PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");

    cString str = G_STRING(OFS_PARM0);
    G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
    PR_CheckEmptyString(str);

    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (!sv.sound_precache[i]) {
            sv.sound_precache[i] = str;
            return;
        }
        if (!strcmp(sv.sound_precache[i], str))
            return;
    }
    PR_RunError("PF_precache_sound: overflow");
}


void PF_precache_model() {
    if (sv.state != ss_loading)
        PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");

    cString str = G_STRING(OFS_PARM0);
    G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
    PR_CheckEmptyString(str);

    for (int i = 0; i < MAX_MODELS; i++) {
        if (!sv.model_precache[i]) {
            sv.model_precache[i] = str;
            sv.models[i] = Mod_ForName(str, true);
            return;
        }
        if (!strcmp(sv.model_precache[i], str))
            return;
    }
    PR_RunError("PF_precache_model: overflow");
}



void PF_precache_file() { // precache_file is only used to copy files with qcc, it does nothing
    G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

/*
==============
PF_changelevel
==============
*/
void PF_changelevel() {
#ifdef QUAKE2
    if (svs.changelevel_issued)        return;
    svs.changelevel_issued = true;

    cString s1 = G_STRING(OFS_PARM0);
    cString s2 = G_STRING(OFS_PARM1);

    if ((int)pr_global_struct->serverflags & (SFL_NEW_UNIT | SFL_NEW_EPISODE))
        Cbuf_AddText(va("changelevel %s %s\n", s1, s2));
    else
        Cbuf_AddText(va("changelevel2 %s %s\n", s1, s2));
#else
    // make sure we don't issue two changelevels
    if (svs.changelevel_issued)
        return;
    svs.changelevel_issued = true;

    cString str = G_STRING(OFS_PARM0);
    Cbuf_AddText(va("changelevel %s\n", str));
#endif
}

