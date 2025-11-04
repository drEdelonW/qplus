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
#include "pr_comp.h"
#include <string.h>
#include "console.h"
#include "server.h"
#include "q_tools.h"
#include "host.h"
#include "angle.h"
#include "world.h"
#include "mathlib.h"
#include "cvar_q1.h"
#include "protocol.h"
#include "msg.h"
#include "sys.h"
#include "common.h"
#include "cmd.h"
#include "cbuf.h"
#include <stdlib.h>

#define RETURN_EDICT(edict) (((int *)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(edict))

/*
===============================================================================

                        BUILT-IN FUNCTIONS

===============================================================================
*/

cString PF_VarString(int first) {
    static char out[256];

    out[0] = 0;
    for (int i = first; i < pr_argc; i++) {
        strcat(out, G_STRING((OFS_PARM0 + i * 3)));
    }
    return out;
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
        pr_strings + pr_xfunction->s_name,
        str);
    edict_p ed = PROG_TO_EDICT(pr_global_struct->self);
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
        pr_strings + pr_xfunction->s_name,
        str);
    edict_p ed = PROG_TO_EDICT(pr_global_struct->self);
    ED_Print(ed);
    ED_Free(ed);

    Host_Error("Program error");
}

/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void PF_makevectors() {
    AngleVectors(
        G_VECTOR(OFS_PARM0),
        pr_global_struct->v_forward,
        pr_global_struct->v_right,
        pr_global_struct->v_up
    );
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
void PF_setorigin() {
    edict_p edict = G_EDICT(OFS_PARM0);
    float_p org = G_VECTOR(OFS_PARM1);
    VectorCopy(org, edict->v.origin);
    SV_LinkEdict(edict, false);
}

void SetMinMaxSize(edict_p edict, float_p min, float_p max, bool rotate) {
    for (int i = 0; i < VECT_DIM; i++)
        if (min[i] > max[i])
            PR_RunError("backwards mins/maxs");

    rotate = false; // FIXME: implement rotation properly again

    vec3_t rmin, rmax;
    if (!rotate) {
        VectorCopy(min, rmin);
        VectorCopy(max, rmax);
    }
    else {
        // find min / max for rotations
        float_p angles = edict->v.angles;

        float a = angles[1] / 180 * M_PI;

        float xvector[2] = {
            cos(a),
            sin(a)
        };
        float yvector[2] = {
            -sin(a),
            cos(a)
        };

        float bounds[2][3];
        VectorCopy(min, bounds[0]);
        VectorCopy(max, bounds[1]);

        rmin[0] = rmin[1] = rmin[2] = 9999;
        rmax[0] = rmax[1] = rmax[2] = -9999;

        vec3_t base;
        for (int i = 0; i <= 1; i++) {
            base[0] = bounds[i][0];
            for (int j = 0; j <= 1; j++) {
                base[1] = bounds[j][1];
                for (int k = 0; k <= 1; k++) {
                    base[2] = bounds[k][2];

                    // transform the point
                    vec3_t transformed = {
                        xvector[0] * base[0] + yvector[0] * base[1],
                        xvector[1] * base[0] + yvector[1] * base[1],
                        base[2]
                    };
                    for (int l = 0; l < VECT_DIM; l++) {
                        if (transformed[l] < rmin[l])       rmin[l] = transformed[l];
                        if (transformed[l] > rmax[l])       rmax[l] = transformed[l];
                    }
                }
            }
        }
    }

    // set derived values
    VectorCopy(rmin, edict->v.mins);
    VectorCopy(rmax, edict->v.maxs);
    VectorSubtract(max, min, edict->v.size);

    SV_LinkEdict(edict, false);
}

/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void PF_setsize() {
    edict_p edict = G_EDICT(OFS_PARM0);
    float_p min = G_VECTOR(OFS_PARM1);
    float_p max = G_VECTOR(OFS_PARM2);
    SetMinMaxSize(edict, min, max, false);
}

/*
=================
PF_setmodel

setmodel(entity, model)
=================
*/
void PF_setmodel() {
    edict_p edict = G_EDICT(OFS_PARM0);
    cString m = G_STRING(OFS_PARM1);

    // check to see if model was properly precached
    cStringArray check = sv.model_precache;
    int i = 0;
    for (; *check; i++, check++)
        if (!strcmp(*check, m))     break;

    if (!*check)        PR_RunError("no precache: %s\n", m);

    edict->v.model = m - pr_strings;
    edict->v.modelindex = i; // SV_ModelIndex (m);

    Model_p mod = sv.models[(int)edict->v.modelindex]; // Mod_ForName (m, true);

    if (mod)    SetMinMaxSize(edict, mod->mins, mod->maxs, true);
    else        SetMinMaxSize(edict, vec3_origin, vec3_origin, true);
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
    int ent_num = G_EDICTNUM(OFS_PARM0);
    cString str = PF_VarString(1);

    if ((ent_num < 1) ||
        (ent_num > svs.maxClients)) {
        Con_Printf("tried to sprint to a non-client\n");
        return;
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
    int entnum = G_EDICTNUM(OFS_PARM0);
    cString str = PF_VarString(1);

    if ((entnum < 1) || (entnum > svs.maxClients)) {
        Con_Printf("tried to sprint to a non-client\n");
        return;
    }

    RmtClient_p client = &svs.clients[entnum - 1];

    MSG_WriteChar(&client->message, svc_centerprint);
    MSG_WriteString(&client->message, str);
}

/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void PF_normalize() {
    float_p value1 = G_VECTOR(OFS_PARM0);
    float new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
    new = sqrt(new);

    vec3_t newvalue;
    if (new == 0)
        newvalue[0] = newvalue[1] = newvalue[2] = 0;
    else {
        new = 1 / new;
        newvalue[0] = value1[0] * new;  // newvalue = value1 * new;
        newvalue[1] = value1[1] * new;
        newvalue[2] = value1[2] * new;
    }

    VectorCopy(newvalue, G_VECTOR(OFS_RETURN));
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
void PF_vlen() {
    float_p value1 = G_VECTOR(OFS_PARM0);

    float new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
    new = sqrt(new);

    G_FLOAT(OFS_RETURN) = new;
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void PF_vectoyaw() {
    float_p value1 = G_VECTOR(OFS_PARM0);

    float yaw;
    if ((value1[1] == 0) &&
        (value1[0] == 0))
        yaw = 0;
    else {
        yaw = (int)(atan2(value1[1], value1[0]) * 180 / M_PI);
        if (yaw < 0)
            yaw += 360;
    }

    G_FLOAT(OFS_RETURN) = yaw;
}

/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
void PF_vectoangles() {
    float yaw, pitch;
    float_p value1 = G_VECTOR(OFS_PARM0);

    if ((value1[1] == 0) &&
        (value1[0] == 0)) {
        yaw = 0;
        if (value1[2] > 0)  pitch = 90;
        else                pitch = 270;
    }
    else {
        yaw = (int)(atan2(value1[1], value1[0]) * 180 / M_PI);
        if (yaw < 0)
            yaw += 360;

        float forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
        pitch = (int)(atan2(value1[2], forward) * 180 / M_PI);
        if (pitch < 0)
            pitch += 360;
    }

    G_FLOAT(OFS_RETURN + 0) = pitch;
    G_FLOAT(OFS_RETURN + 1) = yaw;
    G_FLOAT(OFS_RETURN + 2) = 0;
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
void PF_random() {
    G_FLOAT(OFS_RETURN) = (rand() & 0x7fff) / ((float)0x7fff);
}

/*
=================
PF_particle

particle(origin, color, count)
=================
*/
void PF_particle() {
    float_p org = G_VECTOR(OFS_PARM0);
    float_p dir = G_VECTOR(OFS_PARM1);
    float color = G_FLOAT(OFS_PARM2);
    float count = G_FLOAT(OFS_PARM3);
    SV_StartParticle(org, dir, color, count);
}

/*
=================
PF_ambientsound

=================
*/
void PF_ambientsound() {
    float_p pos = G_VECTOR(OFS_PARM0);
    cString samp = G_STRING(OFS_PARM1);
    float vol = G_FLOAT(OFS_PARM2);
    float attenuation = G_FLOAT(OFS_PARM3);

    // check to see if samp was properly precached
    cStringArray check = sv.sound_precache;
    int soundnum = 0;
    for (; *check; check++, soundnum++)
        if (!strcmp(*check, samp))
            break;

    if (!*check) {
        Con_Printf("no precache: %s\n", samp);
        return;
    }

    // add an svc_spawnambient command to the level signon packet

    MSG_WriteByte(&sv.signon, svc_spawnstaticsound);
    for (int i = 0; i < 3; i++)
        MSG_WriteCoord(&sv.signon, pos[i]);

    MSG_WriteByte(&sv.signon, soundnum);

    MSG_WriteByte(&sv.signon, vol * 255);
    MSG_WriteByte(&sv.signon, attenuation * 64);
}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void PF_sound() {
    edict_p entity = G_EDICT(OFS_PARM0);
    int channel = G_FLOAT(OFS_PARM1);
    cString sample = G_STRING(OFS_PARM2);
    int volume = G_FLOAT(OFS_PARM3) * 255;
    float attenuation = G_FLOAT(OFS_PARM4);

    if ((volume < 0) ||
        (volume > 255))
        Sys_Error("SV_StartSound: volume = %i", volume);

    if ((attenuation < 0) ||
        (attenuation > 4))
        Sys_Error("SV_StartSound: attenuation = %f", attenuation);

    if ((channel < 0) ||
        (channel > 7))
        Sys_Error("SV_StartSound: channel = %i", channel);

    SV_StartSound(entity, channel, sample, volume, attenuation);
}

/*
=================
PF_break

break()
=================
*/
void PF_break() {
    Con_Printf("break statement\n");
    *(int*)-4 = 0; // dump to debugger
    //	PR_RunError ("break statement");
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void PF_traceline() {
    float_p v1 = G_VECTOR(OFS_PARM0);
    float_p v2 = G_VECTOR(OFS_PARM1);
    int nomonsters = G_FLOAT(OFS_PARM2);
    edict_p ent = G_EDICT(OFS_PARM3);

    trace_t trace = SV_Move(v1, vec3_origin, vec3_origin, v2, nomonsters, ent);

    pr_global_struct->trace_allsolid = trace.allsolid;
    pr_global_struct->trace_startsolid = trace.startsolid;
    pr_global_struct->trace_fraction = trace.fraction;
    pr_global_struct->trace_inwater = trace.inwater;
    pr_global_struct->trace_inopen = trace.inopen;
    VectorCopy(trace.endpos, pr_global_struct->trace_endpos);
    VectorCopy(trace.plane.normal, pr_global_struct->trace_plane_normal);
    pr_global_struct->trace_plane_dist = trace.plane.dist;
    if (trace.ent)  pr_global_struct->trace_ent = EDICT_TO_PROG(trace.ent);
    else            pr_global_struct->trace_ent = EDICT_TO_PROG(sv.edicts);
}

#ifdef QUAKE2

void PF_TraceToss() {
    edict_p ent = G_EDICT(OFS_PARM0);
    edict_p ignore = G_EDICT(OFS_PARM1);

    trace_t trace = SV_Trace_Toss(ent, ignore);

    pr_global_struct->trace_allsolid = trace.allsolid;
    pr_global_struct->trace_startsolid = trace.startsolid;
    pr_global_struct->trace_fraction = trace.fraction;
    pr_global_struct->trace_inwater = trace.inwater;
    pr_global_struct->trace_inopen = trace.inopen;
    VectorCopy(trace.endpos, pr_global_struct->trace_endpos);
    VectorCopy(trace.plane.normal, pr_global_struct->trace_plane_normal);
    pr_global_struct->trace_plane_dist = trace.plane.dist;
    if (trace.ent)  pr_global_struct->trace_ent = EDICT_TO_PROG(trace.ent);
    else            pr_global_struct->trace_ent = EDICT_TO_PROG(sv.edicts);
}
#endif

/*
=================
PF_checkpos

Returns true if the given entity can move to the given position from it's
current position by walking or rolling.
FIXME: make work...
scalar checkpos (entity, vector)
=================
*/
void PF_checkpos() {
}

//============================================================================

uint8_t checkpvs[MAX_MAP_LEAFS / 8];

int PF_newcheckclient(int check) {
    // cycle to the next one
    CLAMP(1, check, svs.maxClients);

    int i = (check == svs.maxClients) ? 0 : check + 1;

    edict_p ent;
    for (;; i++) {
        if (i == svs.maxClients + 1)
            i = 1;

        ent = EDICT_NUM(i);

        if (i == check) break; // didn't find anything else

        if ((ent->free) ||
            (ent->v.health <= 0) ||
            ((int)ent->v.flags & FL_NOTARGET)
            ) {
            continue;
        }

        // anything that is a client, or has a client as an enemy
        break;
    }

    // get the PVS for the entity
    vec3_t org;    VectorAdd(ent->v.origin, ent->v.view_ofs, org);
    mLeaf_p leaf = Mod_PointInLeaf(org, sv.worldmodel);
    uint8_p pvs = Mod_LeafPVS(leaf, sv.worldmodel);
    memcpy(checkpvs, pvs, (sv.worldmodel->numleafs + 7) >> 3);

    return i;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define MAX_CHECK 16
int c_invis, c_notvis;
void PF_checkclient() {
    // find a new check if on a new frame
    if (sv.time - sv.lastchecktime >= 0.1) {
        sv.lastcheck = PF_newcheckclient(sv.lastcheck);
        sv.lastchecktime = sv.time;
    }

    // return check if it might be visible
    edict_p ent = EDICT_NUM(sv.lastcheck);
    if (ent->free ||
        (ent->v.health <= 0)) {
        RETURN_EDICT(sv.edicts);
        return;
    }

    // if current entity can't possibly see the check entity, return 0
    edict_p self = PROG_TO_EDICT(pr_global_struct->self);
    vec3_t view; VectorAdd(self->v.origin, self->v.view_ofs, view);
    mLeaf_p leaf = Mod_PointInLeaf(view, sv.worldmodel);
    int l = (leaf - sv.worldmodel->leafs) - 1;
    if ((l < 0) ||
        !(checkpvs[l >> 3] & (1 << (l & 7)))
        ) {
        c_notvis++;
        RETURN_EDICT(sv.edicts);
        return;
    }

    // might be able to see it
    c_invis++;
    RETURN_EDICT(ent);
}

//============================================================================

/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF_stuffcmd() {
    int entnum = G_EDICTNUM(OFS_PARM0);
    if ((entnum < 1) ||
        (entnum > svs.maxClients))
        PR_RunError("Parm 0 not a client");
    cString str = G_STRING(OFS_PARM1);

    RmtClient_p old = remoteClient;
    remoteClient = &svs.clients[entnum - 1];
    Host_ClientCommands("%s", str);
    remoteClient = old;
}

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

localcmd (string)
=================
*/
void PF_localcmd() {
    cString str = G_STRING(OFS_PARM0);
    Cbuf_AddText(str);
}

/*
=================
PF_cvar

float cvar (string)
=================
*/
void PF_cvar() {
    cString str = G_STRING(OFS_PARM0);
    G_FLOAT(OFS_RETURN) = Cvar_VariableValue(str);
}

/*
=================
PF_cvar_set

float cvar (string)
=================
*/
void PF_cvar_set() {
    cString var = G_STRING(OFS_PARM0);
    cString val = G_STRING(OFS_PARM1);
    Cvar_Set(var, val);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void PF_findradius() {
    edict_p chain = (edict_p)sv.edicts;
    float_p org = G_VECTOR(OFS_PARM0);
    float rad = G_FLOAT(OFS_PARM1);

    edict_p ent = NEXT_EDICT(sv.edicts);
    for (int i = 1; i < sv.num_edicts; i++, ent = NEXT_EDICT(ent)) {
        if ((ent->free) ||
            (ent->v.solid == SOLID_NOT))
            continue;

        vec3_t eorg;
        for (int j = 0; j < VECT_DIM; j++) // eorg -= ent->v.origin + (ent->v.mins + ent->v.maxs) * 0.5;
            eorg[j] = org[j] - (ent->v.origin[j] + (ent->v.mins[j] + ent->v.maxs[j]) * 0.5);

        if (Length(eorg) > rad) continue;

        ent->v.chain = EDICT_TO_PROG(chain);
        chain = ent;
    }

    RETURN_EDICT(chain);
}

/*
=========
PF_dprint
=========
*/
void PF_dprint() {
    Con_DPrintf("%s", PF_VarString(0));
}

char pr_string_temp[128];

void PF_ftos() {
    float v = G_FLOAT(OFS_PARM0);

    if (v == (int)v)    sprintf(pr_string_temp, "%d", (int)v);
    else                sprintf(pr_string_temp, "%5.1f", v);
    G_INT(OFS_RETURN) = pr_string_temp - pr_strings;
}

void PF_fabs() {
    float v = G_FLOAT(OFS_PARM0);
    G_FLOAT(OFS_RETURN) = fabs(v);
}

void PF_vtos() {
    sprintf(pr_string_temp, "'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM0)[0], G_VECTOR(OFS_PARM0)[1], G_VECTOR(OFS_PARM0)[2]);
    G_INT(OFS_RETURN) = pr_string_temp - pr_strings;
}

#ifdef QUAKE2
void PF_etos() {
    sprintf(pr_string_temp, "entity %i", G_EDICTNUM(OFS_PARM0));
    G_INT(OFS_RETURN) = pr_string_temp - pr_strings;
}
#endif

void PF_Spawn() {
    edict_p ed = ED_Alloc();
    RETURN_EDICT(ed);
}

void PF_Remove() {
    edict_p ed = G_EDICT(OFS_PARM0);
    ED_Free(ed);
}

// entity (entity start, .string field, string match) find = #5;
void PF_Find()
#ifdef QUAKE2
{
    edict_p first;
    edict_p second;
    edict_p last;

    first = second = last = (edict_p)sv.edicts;
    edict = G_EDICTNUM(OFS_PARM0);
    int f = G_INT(OFS_PARM1);
    cString str = G_STRING(OFS_PARM2);
    if (!str)
        PR_RunError("PF_Find: bad search string");

    for (int edict++; edict < sv.num_edicts; edict++) {
        edict_p ed = EDICT_NUM(edict);
        if (ed->free)
            continue;
        cString t = E_STRING(ed, f);
        if (!t)            continue;
        if (!strcmp(t, str)) {
            if (first == (edict_p)sv.edicts)
                first = ed;
            else if (second == (edict_p)sv.edicts)
                second = ed;
            ed->v.chain = EDICT_TO_PROG(last);
            last = ed;
        }
    }

    if (first != last) {
        if (last != second)     first->v.chain = last->v.chain;
        else                    first->v.chain = EDICT_TO_PROG(last);
        last->v.chain = EDICT_TO_PROG((edict_p)sv.edicts);
        if (second && second != last)
            second->v.chain = EDICT_TO_PROG(last);
    }
    RETURN_EDICT(first);
}
#else
{
    int edict = G_EDICTNUM(OFS_PARM0);
    int f = G_INT(OFS_PARM1);
    cString str = G_STRING(OFS_PARM2);
    if (!str)
        PR_RunError("PF_Find: bad search string");

    for (edict++; edict < sv.num_edicts; edict++) {
        edict_p ed = EDICT_NUM(edict);
        if (ed->free)   continue;
        cString t = E_STRING(ed, f);
        if (!t)         continue;
        if (!strcmp(t, str)) {
            RETURN_EDICT(ed);
            return;
        }
    }

    RETURN_EDICT(sv.edicts);
}
#endif

void PR_CheckEmptyString(cString str) {
    if (str[0] <= ' ')
        PR_RunError("Bad string");
}

void PF_precache_file() { // precache_file is only used to copy files with qcc, it does nothing
    G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
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

void PF_coredump() {
    ED_PrintEdicts();
}

void PF_traceon() {
    pr_trace = true;
}

void PF_traceoff() {
    pr_trace = false;
}

void PF_eprint() {
    ED_PrintNum(G_EDICTNUM(OFS_PARM0));
}

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
void PF_walkmove() {
    edict_p ent = PROG_TO_EDICT(pr_global_struct->self);
    float yaw = G_FLOAT(OFS_PARM0);
    float dist = G_FLOAT(OFS_PARM1);

    if (!((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM))) {
        G_FLOAT(OFS_RETURN) = 0;
        return;
    }

    yaw = yaw * M_PI * 2 / 360;

    vec3_t move = {
        cos(yaw) * dist,
        sin(yaw) * dist,
        0,
    };

    // save program state, because SV_movestep may call other progs
    dFunction_p oldf = pr_xfunction;
    int oldself = pr_global_struct->self;

    G_FLOAT(OFS_RETURN) = SV_movestep(ent, move, true);

    // restore program state
    pr_xfunction = oldf;
    pr_global_struct->self = oldself;
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void PF_droptofloor() {
    edict_p ent = PROG_TO_EDICT(pr_global_struct->self);
    vec3_t end;    VectorCopy(ent->v.origin, end);
    end[2] -= 256;

    trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

    if ((trace.fraction == 1) ||
        trace.allsolid
        )
        G_FLOAT(OFS_RETURN) = 0;
    else {
        VectorCopy(trace.endpos, ent->v.origin);
        SV_LinkEdict(ent, false);
        ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
        ent->v.groundentity = EDICT_TO_PROG(trace.ent);
        G_FLOAT(OFS_RETURN) = 1;
    }
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void PF_lightstyle() {
    int style = G_FLOAT(OFS_PARM0);
    cString val = G_STRING(OFS_PARM1);

    // change the string in sv
    sv.lightstyles[style] = val;

    // send message to all clients on this server
    if (sv.state != ss_active) return;

    RmtClient_p client = svs.clients;
    for (int j = 0; j < svs.maxClients; j++, client++)
        if (client->active || client->spawned) {
            MSG_WriteChar(&client->message, svc_lightstyle);
            MSG_WriteChar(&client->message, style);
            MSG_WriteString(&client->message, val);
        }
}

void PF_rint() {
    float f = G_FLOAT(OFS_PARM0);
    if (f > 0)  G_FLOAT(OFS_RETURN) = (int)(f + 0.5);
    else        G_FLOAT(OFS_RETURN) = (int)(f - 0.5);
}
void PF_floor() {
    G_FLOAT(OFS_RETURN) = floor(G_FLOAT(OFS_PARM0));
}
void PF_ceil() {
    G_FLOAT(OFS_RETURN) = ceil(G_FLOAT(OFS_PARM0));
}

/*
=============
PF_checkbottom
=============
*/
void PF_checkbottom() {
    edict_p ent = G_EDICT(OFS_PARM0);
    G_FLOAT(OFS_RETURN) = SV_CheckBottom(ent);
}

/*
=============
PF_pointcontents
=============
*/
void PF_pointcontents() {
    float_p v = G_VECTOR(OFS_PARM0);
    G_FLOAT(OFS_RETURN) = SV_PointContents(v);
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
void PF_nextent() {
    int i = G_EDICTNUM(OFS_PARM0);
    while (1) {
        i++;
        if (i == sv.num_edicts) { RETURN_EDICT(sv.edicts); return; }
        edict_p ent = EDICT_NUM(i);
        if (!ent->free) { RETURN_EDICT(ent); return; }
    }
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
void PF_aim() {
    edict_p ent = G_EDICT(OFS_PARM0);
    // float speed = G_FLOAT(OFS_PARM1);

    vec3_t start;   VectorCopy(ent->v.origin, start);
    start[2] += 20;

    // try sending a trace straight
    vec3_t dir; VectorCopy(pr_global_struct->v_forward, dir);
    vec3_t end; VectorMA(start, 2048, dir, end);
    trace_t tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
    if (
        tr.ent &&
        (tr.ent->v.takedamage == DAMAGE_AIM) &&
        (!teamplay.value ||
            (ent->v.team <= 0) ||
            (ent->v.team != tr.ent->v.team))) {
        VectorCopy(pr_global_struct->v_forward, G_VECTOR(OFS_RETURN));
        return;
    }

    // try all possible entities
    vec3_t bestdir; VectorCopy(dir, bestdir);
    float bestdist = sv_aim.value;
    edict_p bestent = NULL;

    edict_p check = NEXT_EDICT(sv.edicts);
    for (int i = 1; i < sv.num_edicts; i++, check = NEXT_EDICT(check)) {
        if ((check->v.takedamage != DAMAGE_AIM) ||
            (check == ent) ||
            (teamplay.value &&
                (ent->v.team > 0) &&
                (ent->v.team == check->v.team))) {
            continue; // don't aim at teammate
        }

        for (int j = 0; j < VECT_DIM; j++) {
            end[j] =
                check->v.origin[j] +
                0.5 * (check->v.mins[j] +
                    check->v.maxs[j]);
        }
        VectorSubtract(end, start, dir);
        VectorNormalize(dir);
        float dist = DotProduct(dir, pr_global_struct->v_forward);
        if (dist < bestdist) {
            continue; // to far to turn
        }
        tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
        if (tr.ent == check) { // can shoot at this one
            bestdist = dist;
            bestent = check;
        }
    }

    if (bestent) {
        VectorSubtract(bestent->v.origin, ent->v.origin, dir);
        float dist = DotProduct(dir, pr_global_struct->v_forward);
        VectorScale(pr_global_struct->v_forward, dist, end);
        end[2] = dir[2];
        VectorNormalize(end);
        VectorCopy(end, G_VECTOR(OFS_RETURN));
    }
    else {
        VectorCopy(bestdir, G_VECTOR(OFS_RETURN));
    }
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void PF_changeyaw() {
    edict_p ent = PROG_TO_EDICT(pr_global_struct->self);
    float current = anglemod(ent->v.angles[1]);
    float ideal = ent->v.ideal_yaw;
    float speed = ent->v.yaw_speed;

    if (current == ideal)   return;
    float move = ideal - current;
    if (ideal > current) {
        if (move >= 180)    move = move - 360;
    }
    else {
        if (move <= -180)   move = move + 360;
    }
    if (move > 0)   CLAMP_MORE(move, speed);
    else            CLAMP_LESS(move, -speed);


    ent->v.angles[1] = anglemod(current + move);
}

#ifdef QUAKE2
/*
==============
PF_changepitch
==============
*/
void PF_changepitch() {
    edict_p ent = G_EDICT(OFS_PARM0);
    float current = anglemod(ent->v.angles[0]);
    float ideal = ent->v.idealpitch;
    float speed = ent->v.pitch_speed;

    if (current == ideal)
        return;
    float move = ideal - current;
    if (ideal > current) {
        if (move >= 180)    move = move - 360;
    }
    else {
        if (move <= -180)   move = move + 360;
    }
    if (move > 0) {
        if (move > speed)   move = speed;
    }
    else {
        if (move < -speed)  move = -speed;
    }

    ent->v.angles[0] = anglemod(current + move);
}
#endif

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

typedef enum msg_dest_e {
    MSG_BROADCAST = 0,  // unreliable to all
    MSG_ONE = 1,  // reliable to one (msg_entity)
    MSG_ALL = 2,  // reliable to all
    MSG_INIT = 3   // write to the init string
} msg_dest_e;

sizebuf_p WriteDest() {
    msg_dest_e dest = G_FLOAT(OFS_PARM0);
    switch (dest) {
    case MSG_BROADCAST: return &sv.datagram;

    case MSG_ONE:
        edict_p ent = PROG_TO_EDICT(pr_global_struct->msg_entity);
        int entnum = NUM_FOR_EDICT(ent);
        if ((entnum < 1) || (entnum > svs.maxClients))
            PR_RunError("WriteDest: not a client");
        return &svs.clients[entnum - 1].message;

    case MSG_ALL:       return &sv.reliable_datagram;
    case MSG_INIT:      return &sv.signon;
    default:            PR_RunError("WriteDest: bad destination");  break;
    }

    return NULL;
}

void PF_WriteByte() {
    MSG_WriteByte(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteChar() {
    MSG_WriteChar(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteShort() {
    MSG_WriteShort(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteLong() {
    MSG_WriteLong(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteAngle() {
    MSG_WriteAngle(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteCoord() {
    MSG_WriteCoord(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteString() {
    MSG_WriteString(WriteDest(), G_STRING(OFS_PARM1));
}

void PF_WriteEntity() {
    MSG_WriteShort(WriteDest(), G_EDICTNUM(OFS_PARM1));
}

//=============================================================================

void PF_makestatic() {
    edict_p ent = G_EDICT(OFS_PARM0);
    MSG_WriteByte(&sv.signon, svc_spawnstatic);
    MSG_WriteByte(&sv.signon, SV_ModelIndex(pr_strings + ent->v.model));
    MSG_WriteByte(&sv.signon, ent->v.frame);
    MSG_WriteByte(&sv.signon, ent->v.colormap);
    MSG_WriteByte(&sv.signon, ent->v.skin);
    for (int i = 0; i < VECT_DIM; i++) {
        MSG_WriteCoord(&sv.signon, ent->v.origin[i]);
        MSG_WriteAngle(&sv.signon, ent->v.angles[i]);
    }

    // throw the entity away now
    ED_Free(ent);
}

//=============================================================================

/*
==============
PF_setspawnparms
==============
*/
void PF_setspawnparms() {
    edict_p ent = G_EDICT(OFS_PARM0);
    int i = NUM_FOR_EDICT(ent);
    if ((i < 1) ||
        (i > svs.maxClients))
        PR_RunError("Entity is not a client");

    // copy spawn parms out of the RmtClient_t
    RmtClient_p client = svs.clients + (i - 1);

    for (int i = 0; i < NUM_SPAWN_PARMS; i++)
        (&pr_global_struct->parm1)[i] = client->spawn_parms[i];
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

#ifdef QUAKE2

#define CONTENT_WATER -3
#define CONTENT_SLIME -4
#define CONTENT_LAVA -5

#define FL_IMMUNE_WATER 131072
#define FL_IMMUNE_SLIME 262144
#define FL_IMMUNE_LAVA 524288

#define CHAN_VOICE 2
#define CHAN_BODY 4

#define ATTN_NORM 1

void PF_WaterMove() {
    edict_p self;
    int flags;
    int waterlevel;
    int watertype;
    float drownlevel;
    float damage = 0.0;

    self = PROG_TO_EDICT(pr_global_struct->self);

    if (self->v.movetype == MOVETYPE_NOCLIP) {
        self->v.air_finished = sv.time + 12;
        G_FLOAT(OFS_RETURN) = damage;
        return;
    }

    if (self->v.health < 0) {
        G_FLOAT(OFS_RETURN) = damage;
        return;
    }

    if (self->v.deadflag == DEAD_NO)
        drownlevel = 3;
    else
        drownlevel = 1;

    flags = (int)self->v.flags;
    waterlevel = (int)self->v.waterlevel;
    watertype = (int)self->v.watertype;

    if (!(flags & (FL_IMMUNE_WATER + FL_GODMODE)))
        if (((flags & FL_SWIM) && (waterlevel < drownlevel)) || (waterlevel >= drownlevel)) {
            if (self->v.air_finished < sv.time)
                if (self->v.pain_finished < sv.time) {
                    self->v.dmg = self->v.dmg + 2;
                    if (self->v.dmg > 15)
                        self->v.dmg = 10;
                    //					T_Damage (self, world, world, self.dmg, 0, FALSE);
                    damage = self->v.dmg;
                    self->v.pain_finished = sv.time + 1.0;
                }
        }
        else {
            if (self->v.air_finished < sv.time)
                //				sound (self, CHAN_VOICE, "player/gasp2.wav", 1, ATTN_NORM);
                SV_StartSound(self, CHAN_VOICE, "player/gasp2.wav", 255, ATTN_NORM);
            else if (self->v.air_finished < sv.time + 9)
                //				sound (self, CHAN_VOICE, "player/gasp1.wav", 1, ATTN_NORM);
                SV_StartSound(self, CHAN_VOICE, "player/gasp1.wav", 255, ATTN_NORM);
            self->v.air_finished = sv.time + 12.0;
            self->v.dmg = 2;
        }

    if (!waterlevel) {
        if (flags & FL_INWATER) {
            // play leave water sound
            //			sound (self, CHAN_BODY, "misc/outwater.wav", 1, ATTN_NORM);
            SV_StartSound(self, CHAN_BODY, "misc/outwater.wav", 255, ATTN_NORM);
            self->v.flags = (float)(flags & ~FL_INWATER);
        }
        self->v.air_finished = sv.time + 12.0;
        G_FLOAT(OFS_RETURN) = damage;
        return;
    }

    if (watertype == CONTENT_LAVA) { // do damage
        if (!(flags & (FL_IMMUNE_LAVA + FL_GODMODE)))
            if (self->v.dmgtime < sv.time) {
                if (self->v.radsuit_finished < sv.time)
                    self->v.dmgtime = sv.time + 0.2;
                else
                    self->v.dmgtime = sv.time + 1.0;
                //				T_Damage (self, world, world, 10*self.waterlevel, 0, TRUE);
                damage = (float)(10 * waterlevel);
            }
    }
    else if (watertype == CONTENT_SLIME) { // do damage
        if (!(flags & (FL_IMMUNE_SLIME + FL_GODMODE)))
            if (self->v.dmgtime < sv.time && self->v.radsuit_finished < sv.time) {
                self->v.dmgtime = sv.time + 1.0;
                //				T_Damage (self, world, world, 4*self.waterlevel, 0, TRUE);
                damage = (float)(4 * waterlevel);
            }
    }

    if (!(flags & FL_INWATER)) {

        // player enter water sound
        if (watertype == CONTENT_LAVA)
            //			sound (self, CHAN_BODY, "player/inlava.wav", 1, ATTN_NORM);
            SV_StartSound(self, CHAN_BODY, "player/inlava.wav", 255, ATTN_NORM);
        if (watertype == CONTENT_WATER)
            //			sound (self, CHAN_BODY, "player/inh2o.wav", 1, ATTN_NORM);
            SV_StartSound(self, CHAN_BODY, "player/inh2o.wav", 255, ATTN_NORM);
        if (watertype == CONTENT_SLIME)
            //			sound (self, CHAN_BODY, "player/slimbrn2.wav", 1, ATTN_NORM);
            SV_StartSound(self, CHAN_BODY, "player/slimbrn2.wav", 255, ATTN_NORM);

        self->v.flags = (float)(flags | FL_INWATER);
        self->v.dmgtime = 0;
    }

    if (!(flags & FL_WATERJUMP)) {
        //		self.velocity = self.velocity - 0.8*self.waterlevel*frametime*self.velocity;
        VectorMA(self->v.velocity, -0.8 * self->v.waterlevel * host_frametime, self->v.velocity, self->v.velocity);
    }

    G_FLOAT(OFS_RETURN) = damage;
}

void PF_sin() {
    G_FLOAT(OFS_RETURN) = sin(G_FLOAT(OFS_PARM0));
}

void PF_cos() {
    G_FLOAT(OFS_RETURN) = cos(G_FLOAT(OFS_PARM0));
}

void PF_sqrt() {
    G_FLOAT(OFS_RETURN) = sqrt(G_FLOAT(OFS_PARM0));
}
#endif

void PF_Fixme() {
    PR_RunError("unimplemented bulitin");
}

builtin_t pr_builtin[] = {
    PF_Fixme,
    PF_makevectors,	   // void(entity e)	makevectors 		= #1;
    PF_setorigin,	   // void(entity e, vector o) setorigin	= #2;
    PF_setmodel,	   // void(entity e, string m) setmodel	= #3;
    PF_setsize,		   // void(entity e, vector min, vector max) setsize = #4;
    PF_Fixme,		   // void(entity e, vector min, vector max) setabssize = #5;
    PF_break,		   // void() break						= #6;
    PF_random,		   // float() random						= #7;
    PF_sound,		   // void(entity e, float chan, string samp) sound = #8;
    PF_normalize,	   // vector(vector v) normalize			= #9;
    PF_error,		   // void(string e) error				= #10;
    PF_objerror,	   // void(string e) objerror				= #11;
    PF_vlen,		   // float(vector v) vlen				= #12;
    PF_vectoyaw,	   // float(vector v) vectoyaw		= #13;
    PF_Spawn,		   // entity() spawn						= #14;
    PF_Remove,		   // void(entity e) remove				= #15;
    PF_traceline,	   // float(vector v1, vector v2, float tryents) traceline = #16;
    PF_checkclient,	   // entity() clientlist					= #17;
    PF_Find,		   // entity(entity start, .string fld, string match) find = #18;
    PF_precache_sound, // void(string str) precache_sound		= #19;
    PF_precache_model, // void(string str) precache_model		= #20;
    PF_stuffcmd,	   // void(entity client, string str)stuffcmd = #21;
    PF_findradius,	   // entity(vector org, float rad) findradius = #22;
    PF_bprint,		   // void(string str) bprint				= #23;
    PF_sprint,		   // void(entity client, string str) sprint = #24;
    PF_dprint,		   // void(string str) dprint				= #25;
    PF_ftos,		   // void(string str) ftos				= #26;
    PF_vtos,		   // void(string str) vtos				= #27;
    PF_coredump,
    PF_traceon,
    PF_traceoff,
    PF_eprint,	 // void(entity e) debug print an entire entity
    PF_walkmove, // float(float yaw, float dist) walkmove
    PF_Fixme,	 // float(float yaw, float dist) walkmove
    PF_droptofloor,
    PF_lightstyle,
    PF_rint,
    PF_floor,
    PF_ceil,
    PF_Fixme,
    PF_checkbottom,
    PF_pointcontents,
    PF_Fixme,
    PF_fabs,
    PF_aim,
    PF_cvar,
    PF_localcmd,
    PF_nextent,
    PF_particle,
    PF_changeyaw,
    PF_Fixme,
    PF_vectoangles,

    PF_WriteByte,
    PF_WriteChar,
    PF_WriteShort,
    PF_WriteLong,
    PF_WriteCoord,
    PF_WriteAngle,
    PF_WriteString,
    PF_WriteEntity,

#ifdef QUAKE2
        PF_sin,
        PF_cos,
        PF_sqrt,
        PF_changepitch,
        PF_TraceToss,
        PF_etos,
        PF_WaterMove,
#else
        PF_Fixme,
        PF_Fixme,
        PF_Fixme,
        PF_Fixme,
        PF_Fixme,
        PF_Fixme,
        PF_Fixme,
#endif

        SV_MoveToGoal,
        PF_precache_file,
        PF_makestatic,

        PF_changelevel,
        PF_Fixme,

        PF_cvar_set,
        PF_centerprint,

        PF_ambientsound,

        PF_precache_model,
        PF_precache_sound, // precache_sound2 is different only for qcc
        PF_precache_file,

        PF_setspawnparms
};

builtin_t* pr_builtins = pr_builtin;
int pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);
