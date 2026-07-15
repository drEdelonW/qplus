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
#include "BBox_tools.h"

void PF_Spawn() { RETURN_EDICT(ED_Alloc()); return; }
void PF_Remove() { ED_Free(G_EDICT(OFS_PARM0)); }

// entity (entity start, .string field, string match) find = #5;
void PF_Find() {
#ifdef QUAKE2
    edict_p first = ED_GetEDictByIdx(EdictWorld);
    edict_p second = first;
    edict_p last = second;
    EdIdx edict = G_EDICTNUM(OFS_PARM0);
    qVmString_t qStr = G_INT(OFS_PARM1);
    cString str = G_STRING(OFS_PARM2);
    if (!str)
        PR_RunError("PF_Find: bad search string");

    for (edict++; edict < GetEdNum(); edict++) {
        edict_p ed = ED_GetEDictByIdx(edict);
        if (!ed->inUse)   continue;

        cString cStr = E_STRING(ed, qStr);
        if (!cStr)            continue;
        if (!strcmp(cStr, str)) {
            /**/ if (first == ED_GetEDictByIdx(EdictWorld))   first = ed;
            else if (second == ED_GetEDictByIdx(EdictWorld))  second = ed;
            ed->v.chain = ED_GetEDictOffs(last);
            last = ed;
        }
    }

    if (first != last) {
        if (last != second)     first->v.chain = last->v.chain;
        else                    first->v.chain = ED_GetEDictOffs(last);
        last->v.chain = EdictWorld;
        if (second &&
            (second != last)
            )
            second->v.chain = ED_GetEDictOffs(last);
    }
    RETURN_EDICT(first); return;
#else
    EdIdx edict = G_EDICTNUM(OFS_PARM0);
    qVmString_t qStr = G_INT(OFS_PARM1);
    cString str = G_STRING(OFS_PARM2);
    if (!str)
        PR_RunError("PF_Find: bad search string");

    for (edict++; edict < GetEdNum(); edict++) {
        edict_p ed = ED_GetEDictByIdx(edict);
        if (!ed->inUse)   continue;
        cString cStr = E_STRING(ed, qStr);
        if (!cStr)         continue;
        if (!strcmp(cStr, str)
            ) {
            RETURN_EDICT(ed); return;
        }
    }

    RETURN_EDICT(ED_GetEDictByIdx(EdictWorld)); return;
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
    edict_p chain = ED_GetEDictByIdx(EdictWorld);
    for (EdIdx idx = EdictPlayer1; idx < GetEdNum(); idx++) {
        edict_p ent = ED_GetEDictByIdx(idx);

        if ((!ent->inUse) ||
            (ent->v.solid == SOLID_NOT)
            )   continue;

        if (
            Length(
                VectorSubtract(
                    /*origin*/G_VECTOR(OFS_PARM0), VectorAdd(
                        ent->v.origin, BBoxMid(EvBBox(&ent->v))
                    )
                )
            ) > /*radius*/G_FLOAT(OFS_PARM1)
            )   continue;

        ent->v.chain = ED_GetEDictOffs(chain);
        chain = ent;
    }

    RETURN_EDICT(chain); return;
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
        if (i == GetEdNum()) {
            RETURN_EDICT(ED_GetEDictByIdx(EdictWorld)); return;
        }
        edict_p ent = ED_GetEDictByIdx(i);
        if (ent->inUse) {
            RETURN_EDICT(ent); return;
        }
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
    edict->v.origin = G_VECTOR(OFS_PARM1);
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
    edict->v.size = BBoxSize(bb);
#else
    EvSetBBox(&edict->v, bb);
#endif
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
    SetMinMaxSize(
        G_EDICT(OFS_PARM0),
        BBoxFromVec3(G_VECTOR(OFS_PARM1), G_VECTOR(OFS_PARM2)),
        false
    );
}

/*
=================
PF_setmodel

setmodel(entity, model)
=================
*/
void PF_setmodel() {
    cString m = G_STRING(OFS_PARM1);

    // check to see if model was properly precached
    cStringArray check = sv.model_precache;
    int i = 0;
    for (; *check; i++, check++)
        if (!strcmp(*check, m))     break;

    if (!*check)        PR_RunError("no precache: %s\n", m);

    edict_p edict = G_EDICT(OFS_PARM0);
    edict->v.model = PR_SetQString(m);
    edict->v.modelindex = (float)i; // SV_ModelIndex (m);

    Model_p mod = sv.models[(int)edict->v.modelindex]; // Mod_ForName (m, true);

    if (mod)    SetMinMaxSize(edict, mod->BB, true);
    else        SetMinMaxSize(edict, bbZero, true);
}


void PF_makestatic() {
    edict_p ent = G_EDICT(OFS_PARM0);
    MSG_WriteByte(&sv.signon, svc_spawnstatic); {
        MSG_WriteByte(&sv.signon, (uint8_t)SV_ModelIndex(PR_GetQString(ent->v.model)));
        MSG_WriteByte(&sv.signon, (uint8_t)ent->v.frame);
        MSG_WriteByte(&sv.signon, (uint8_t)ent->v.colormap);
        MSG_WriteByte(&sv.signon, (uint8_t)ent->v.skin);
        for (int i = 0; i < VECT_DIM; i++) {
            MSG_WriteCoord(&sv.signon, ent->v.origin.v[i]);
            MSG_WriteAngle(&sv.signon, ent->v.angles.v[i]);
        }
    }
    ED_Free(ent);   // throw the entity away now
}



void PF_setspawnparms() {
    EdIdx idx = ED_GetEDictIdx(G_EDICT(OFS_PARM0));
    if ((idx < EdictPlayer1) ||
        (idx > GetSvMaxClients())
        )   PR_RunError("Entity is not a client");

    // copy spawn parms out of the RmtClient_t
    RmtClient_p client = svs.clients + (idx - EdictPlayer1);

    for (int i = 0; i < NUM_SPAWN_PARMS; i++)
        (&pGame()->parm1)[i] = client->spawn_parms[i];
}


