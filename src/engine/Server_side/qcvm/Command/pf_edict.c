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
#include "Edict.h"
#include <string.h>
#include "world.h"
#include "server.h"
#include "server_priv.h"
#include "GameRule.h"
#include "msg.h"
#include "protocol.h"
#include "q_tools.h"
#include "LeafModel.h"

void PF_Spawn() {
    edict_p ed = ED_Alloc();
    RETURN_EDICT(ed);
}

void PF_Remove() {
    edict_p ed = G_EDICT(OFS_PARM0);
    ED_Free(ed);
}

// entity (entity start, .string field, string match) find = #5;
void PF_Find() {
#ifdef QUAKE2
    edict_p first = Edicts;
    edict_p second = first;
    edict_p last = second;
    EdIdx edict = G_EDICTNUM(OFS_PARM0);
    int f = G_INT(OFS_PARM1);
    cString str = G_STRING(OFS_PARM2);
    if (!str)
        PR_RunError("PF_Find: bad search string");

    for (edict++; edict < GetEdNum(); edict++) {
        edict_p ed = ED_GetEDictByIdx(edict);
        if (ed->free)
            continue;
        cString t = E_STRING(ed, f);
        if (!t)            continue;
        if (!strcmp(t, str)) {
            if (first == Edicts)        first = ed;
            else if (second == Edicts)  second = ed;
            ed->v.chain = ED_GetEDictOffs(last);
            last = ed;
        }
    }

    if (first != last) {
        if (last != second)     first->v.chain = last->v.chain;
        else                    first->v.chain = ED_GetEDictOffs(last);
        last->v.chain = ED_GetEDictOffs(Edicts);
        if (second &&
            (second != last)
            )
            second->v.chain = ED_GetEDictOffs(last);
    }
    RETURN_EDICT(first);
#else
    EdIdx edict = G_EDICTNUM(OFS_PARM0);
    qVmString_t f = G_INT(OFS_PARM1);
    cString str = G_STRING(OFS_PARM2);
    if (!str)
        PR_RunError("PF_Find: bad search string");

    for (edict++; edict < GetEdNum(); edict++) {
        edict_p ed = ED_GetEDictByIdx(edict);
        if (ed->free)   continue;
        cString t = E_STRING(ed, f);
        if (!t)         continue;
        if (!strcmp(t, str)
            ) {
            RETURN_EDICT(ed);
            return;
        }
    }

    RETURN_EDICT(Edicts);
#endif
}


/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void PF_findradius() {
    edict_p chain = Edicts;
    vec3_t org = G_VECTOR(OFS_PARM0);
    float rad = G_FLOAT(OFS_PARM1);

    for (EdIdx e = EdictPlayer1; e < GetEdNum(); e++) {
        edict_p ent = ED_GetEDictByIdx(e);

        if ((ent->free) ||
            (ent->v.solid == SOLID_NOT)
            )   continue;

        vec3_t eorg = VectorSubtract(org,
            VectorAdd(ent->v.origin,
                VectorScale(VectorAdd(ent->v.mins, ent->v.maxs), 0.5f)
            )
        );

        if (Length(eorg) > rad) continue;

        ent->v.chain = ED_GetEDictOffs(chain);
        chain = ent;
    }

    RETURN_EDICT(chain);
}


/*
=============
PF_nextent

entity nextent(entity)
=============
*/
void PF_nextent() {
    EdIdx i = G_EDICTNUM(OFS_PARM0);
    while (1) {
        i++;
        if (i == GetEdNum()) { RETURN_EDICT(Edicts); return; }
        edict_p ent = ED_GetEDictByIdx(i);
        if (!ent->free) { RETURN_EDICT(ent); return; }
    }
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
    vec3_t org = G_VECTOR(OFS_PARM1);
    edict->v.origin = org;
    SV_LinkEdict(edict, false);
}



void SetMinMaxSize(edict_p edict, BBox_t bb, bool rotate) {
    if (!BBoxIsValid(bb))   PR_RunError("backwards mins/maxs");

#if 0   // disabled because no rotation on map available
    rotate = false; // FIXME: implement rotation properly again
    BBox_t rbb;
    if (!rotate) { rbb = bb; }
    else {
        // find min / max for rotations
        float a = DEG2RAD(edict->v.angles.yaw);
        vec3_t xvector = { .x = cosf(a), .y = sinf(a), .z = 0.f };
        vec3_t yvector = { .x = -sinf(a), .y = cosf(a), .z = 0.f };

        vec3_t bounds[2] = { bb.mins, bb.maxs };
        rbb = bbNull;

        vec3_t base;
        for (int i = 0; i <= 1; i++) {
            base.x = bounds[i].x;
            for (int j = 0; j <= 1; j++) {
                base.y = bounds[j].y;
                for (int k = 0; k <= 1; k++) {
                    base.z = bounds[k].z;

                    // transform the point
                    vec3_t transformed = {
                        .x = (xvector.x * base.x) + (yvector.x * base.y),
                        .y = (xvector.y * base.x) + (yvector.y * base.y),
                        .z = base.z
                    };
                    BBoxExpandPt(&rbb, transformed);
                }
            }
        }
    }

    // set derived values
    edict->v.mins = rbb.mins;
    edict->v.maxs = rbb.maxs;
#else
    edict->v.mins = bb.mins;
    edict->v.maxs = bb.maxs;
#endif
    edict->v.size = BBoxSize(bb);

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
    BBox_t bb = {
        .mins = G_VECTOR(OFS_PARM1),
        .maxs = G_VECTOR(OFS_PARM2)
    };

    SetMinMaxSize(edict, bb, false);
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

    edict->v.model = PR_SetQString(m);
    edict->v.modelindex = (float)i; // SV_ModelIndex (m);

    Model_p mod = sv.models[(int)edict->v.modelindex]; // Mod_ForName (m, true);

    if (mod)    SetMinMaxSize(edict, mod->BB, true);
    else        SetMinMaxSize(edict, bbZero, true);
}


void PF_makestatic() {
    edict_p ent = G_EDICT(OFS_PARM0);
    MSG_WriteByte(&sv.signon, svc_spawnstatic);
    MSG_WriteByte(&sv.signon, (uint8_t)SV_ModelIndex(PR_GetQString(ent->v.model)));
    MSG_WriteByte(&sv.signon, (uint8_t)ent->v.frame);
    MSG_WriteByte(&sv.signon, (uint8_t)ent->v.colormap);
    MSG_WriteByte(&sv.signon, (uint8_t)ent->v.skin);
    for (int i = 0; i < VECT_DIM; i++) {
        MSG_WriteCoord(&sv.signon, ent->v.origin.v[i]);
        MSG_WriteAngle(&sv.signon, ent->v.angles.v[i]);
    }

    // throw the entity away now
    ED_Free(ent);
}


/*
==============
PF_setspawnparms
==============
*/
void PF_setspawnparms() {
    edict_p ent = G_EDICT(OFS_PARM0);
    uint32_t i = ED_GetEDictIdx(ent);
    if ((i < 1) ||
        (i > GetSvMaxClients()))
        PR_RunError("Entity is not a client");

    // copy spawn parms out of the RmtClient_t
    RmtClient_p client = svs.clients + (i - 1);

    for (int i = 0; i < NUM_SPAWN_PARMS; i++)
        (&pr_global_struct->parm1)[i] = client->spawn_parms[i];
}


//============================================================================


static uint8_t _checkPvs[MAX_MAP_LEAFS / 8];

uint8_t PF_newcheckclient(uint8_t check) {
    // cycle to the next one
    CLAMP(1u, &check, GetSvMaxClients());

    uint8_t i = (check == GetSvMaxClients()) ? 0 : (check + 1);

    edict_p ent;
    for (;; i++) {
        if (i == GetSvMaxClients() + 1)
            i = 1;

        ent = ED_GetEDictByIdx(i);

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
    vec3_t org = VectorAdd(ent->v.origin, ent->v.view_ofs);
    mLeaf_p leaf = Mod_PointInLeaf(org, sv.worldmodel);
    uint8_p pvs = Mod_LeafPVS(leaf, sv.worldmodel);
    memcpy(_checkPvs, pvs, (sv.worldmodel->numleafs + 7) >> 3);

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

name checkclient()
=================
*/
// #define MAX_CHECK 16
// int c_invis, c_notvis;
void PF_checkclient() {
    // find a new check if on a new frame
    if ((SV_GetTime() - sv.lastchecktime) >= 0.1) {
        sv.lastcheck = PF_newcheckclient(sv.lastcheck);
        sv.lastchecktime = SV_GetTime();
    }

    // return check if it might be visible
    edict_p ent = ED_GetEDictByIdx(sv.lastcheck);
    if ((ent->free) ||
        (ent->v.health <= 0)
        ) {
        RETURN_EDICT(Edicts);
        return;
    }

    // if current entity can't possibly see the check entity, return 0
    edict_p self = ED_GetEDictByOffs(pr_global_struct->self);
    vec3_t view = VectorAdd(self->v.origin, self->v.view_ofs);
    mLeaf_p leaf = Mod_PointInLeaf(view, sv.worldmodel);
    int l = (leaf - sv.worldmodel->leafs) - 1;
    if ((l < 0) ||
        !(_checkPvs[EIGHTH(l)] & (1 << (l & 7)))
        ) {
        // c_notvis++;
        RETURN_EDICT(Edicts);
        return;
    }

    // might be able to see it
    // c_invis++;
    RETURN_EDICT(ent);
}

