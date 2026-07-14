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
#include "sv_phys.h"
#include "Edict.h"
#include "world.h"
#include "server_priv.h"
#include "cvar_q1.h"
#include "q_tools.h"
#include "BBox_tools.h"
#include "trace.h"

dFunction_p pr_xFunction;

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
void PF_walkmove() {
    edict_p ent = ED_GetEDictByOffs(GV_pGame()->self);
    if (!((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM))) {
        G_FLOAT(OFS_RETURN) = 0;    return;
    }

    Angle_t yaw = DEG2RAD(G_FLOAT(OFS_PARM0));
    vec3_t move = {
        .x = cosf(yaw),
        .y = sinf(yaw),
    };

    // save program state, because SV_movestep may call other progs
    dFunction_p oldf = pr_xFunction;    int oldself = GV_pGame()->self; { // PR_PUSH
        G_FLOAT(OFS_RETURN) = SV_movestep(ent, VectorScale(move, G_FLOAT(OFS_PARM1)), true);
    } pr_xFunction = oldf;    GV_pGame()->self = oldself; // PR_POP // restore program state
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void PF_droptofloor() {
    edict_p ent = ED_GetEDictByOffs(GV_pGame()->self);
    vec3_t end = ent->v.origin; { end.z -= 256.f; /* down */ }
    trace_t trace = SV_Move(ent->v.origin, EvBBox(&ent->v), end, MOVE_NORMAL, ent);

    if ((trace.fraction == 1) ||
        (trace.allsolid)
        )   G_FLOAT(OFS_RETURN) = 0;
    else {
        ent->v.origin = trace.endpos;
        SV_LinkEdict(ent, false);
        ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
        ent->v.groundentity = ED_GetEDictOffs(trace.pEnt);
        G_FLOAT(OFS_RETURN) = 1;
    }
}



void PF_checkbottom() {
    G_FLOAT(OFS_RETURN) = SV_CheckBottom(G_EDICT(OFS_PARM0));
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
#include "PhyMoveType.h"
void PF_traceline() {
    trace_t trace = SV_Move(
        G_VECTOR(OFS_PARM0), bbZero, G_VECTOR(OFS_PARM1),
        (phymovetype_t)G_FLOAT(OFS_PARM2),
        G_EDICT(OFS_PARM3)
    ); {
        trace.pEnt = (trace.pEnt) ? trace.pEnt : Edicts;
    }
    {
        GV_pGame()->trace_allsolid    /**/ = trace.allsolid;
        GV_pGame()->trace_startsolid  /**/ = trace.startsolid;
        GV_pGame()->trace_fraction    /**/ = trace.fraction;
        GV_pGame()->trace_endpos      /**/ = trace.endpos;
        GV_pGame()->trace_plane_normal/**/ = trace.plane.normal;
        GV_pGame()->trace_plane_dist  /**/ = trace.plane.dist;
        GV_pGame()->trace_ent         /**/ = ED_GetEDictOffs(trace.pEnt);
        GV_pGame()->trace_inopen      /**/ = trace.inopen;
        GV_pGame()->trace_inwater     /**/ = trace.inwater;
    }
}


#ifdef QUAKE2
void PF_TraceToss() {
    trace_t trace = SV_Trace_Toss(
        G_EDICT(OFS_PARM0),
        G_EDICT(OFS_PARM1)
    );
    trace.pEnt = (trace.pEnt) ? trace.pEnt : Edicts;
    {
        GV_pGame()->trace_allsolid    /**/ = trace.allsolid;
        GV_pGame()->trace_startsolid  /**/ = trace.startsolid;
        GV_pGame()->trace_fraction    /**/ = trace.fraction;
        GV_pGame()->trace_endpos      /**/ = trace.endpos;
        GV_pGame()->trace_plane_normal/**/ = trace.plane.normal;
        GV_pGame()->trace_plane_dist  /**/ = trace.plane.dist;
        GV_pGame()->trace_ent         /**/ = ED_GetEDictOffs(trace.pEnt);
        GV_pGame()->trace_inopen      /**/ = trace.inopen;
        GV_pGame()->trace_inwater     /**/ = trace.inwater;
    }
}
#endif

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
void PF_aim() {
    edict_p ent = G_EDICT(OFS_PARM0);   // float speed = G_FLOAT(OFS_PARM1);
    vec3_t start = ent->v.origin; {
        start.z += 20.0f;
    }

    // try sending a trace straight
    vec3_t dir = GV_pGame()->v_forward;
    trace_t tr = SV_Move(
        start, bbZero,
        VectorMA(start, 2048, dir),
        MOVE_NORMAL, ent
    );
    if (
        (tr.pEnt) &&
        (tr.pEnt->v.takedamage == DAMAGE_AIM) &&
        (
            !(teamplay.value) ||
            (ent->v.team <= 0) ||
            (ent->v.team != tr.pEnt->v.team))
        ) {
        G_VECTOR(OFS_RETURN) = GV_pGame()->v_forward; return;
    }

    // try all possible entities
    vec3_t bestdir = dir;
    float bestdist = sv_aim.value;
    edict_p bestent = NULL;

    for (EdIdx e = EdictPlayer1; e < GetEdNum(); e++) {
        edict_p check = ED_GetEDictByIdx(e);

        if ((check->v.takedamage != DAMAGE_AIM) ||
            (check == ent) ||
            (
                (teamplay.value) &&
                (ent->v.team > 0) &&
                (ent->v.team == check->v.team))
            )   continue; // don't aim at teammate

        vec3_t end = VectorAdd(
            check->v.origin,
            BBoxMid(EvBBox(&check->v))
        );
        vec3_t dir = VectorSubtract(end, start);
        VectorNormalize(&dir);
        float dist = DotProduct(dir, GV_pGame()->v_forward);
        if (dist < bestdist)
            continue; // to far to turn

        tr = SV_Move(start, bbZero, end, MOVE_NORMAL, ent);
        if (tr.pEnt == check) { // can shoot at this one
            bestdist = dist;
            bestent = check;
        }
    }

    if (bestent) {
        vec3_t dir = VectorSubtract(bestent->v.origin, ent->v.origin);
        vec3_t end = VectorScale(
            GV_pGame()->v_forward,
            DotProduct(dir, GV_pGame()->v_forward)
        ); {
            end.z = dir.z;
            VectorNormalize(&end);
        }
        G_VECTOR(OFS_RETURN) = end; return;
    }
    else { G_VECTOR(OFS_RETURN) = bestdir; return; }
}


/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void PF_changeyaw() {
    edict_p ent = ED_GetEDictByOffs(GV_pGame()->self);
    float current = anglemod(ent->v.angles.yaw);
    float rotSpeed = ent->v.yaw_speed;

    float rot = angledelta(ent->v.ideal_yaw - current);
    if (rot == 0.f)    return;
    ClampInRange(-rotSpeed, &rot, rotSpeed);
    ent->v.angles.yaw = anglemod(current + rot);
}


#ifdef QUAKE2
void PF_changepitch() {
    edict_p ent = G_EDICT(OFS_PARM0);
    float current = anglemod(ent->v.angles.pitch);
    float rotSpeed = ent->v.pitch_speed;

    float rot = angledelta(ent->v.idealpitch - current);
    if (rot == 0.f)    return;
    ClampInRange(-rotSpeed, &rot, rotSpeed);
    ent->v.angles.pitch = anglemod(current + rot);
}
#endif


#ifdef QUAKE2
void PF_WaterMove() {
    float damage = 0.f;
    edict_p self = ED_GetEDictByOffs(GV_pGame()->self);

    if (self->v.movetype == MOVETYPE_NOCLIP) {
        self->v.air_finished = SV_GetTime() + 12.f;
        G_FLOAT(OFS_RETURN) = damage;
        return;
    }

    if (self->v.health < 0) {
        G_FLOAT(OFS_RETURN) = damage;
        return;
    }

    float drownlevel = (self->v.deadflag == DEAD_NO) ? 3.f : 1.f;

    int flags = (int)self->v.flags;
    WaterLevel_t waterlevel = (int)self->v.waterlevel;
    contents_t watertype = (int)self->v.watertype;

    if (!(flags & (FL_IMMUNE_WATER + FL_GODMODE)))
        if (
            (
                (flags & FL_SWIM) &&
                (waterlevel < drownlevel)
                ) ||
            (waterlevel >= drownlevel)
            ) {
            if (self->v.air_finished < SV_GetTime())
                if (self->v.pain_finished < SV_GetTime()) {
                    self->v.dmg = self->v.dmg + 2;
                    if (self->v.dmg > 15.f)   self->v.dmg = 10.f;
                    //     T_Damage (self, world, world, self.dmg, 0, FALSE);
                    damage = self->v.dmg;
                    self->v.pain_finished = SV_GetTime() + 1.0;
                }
        }
        else {
            /* */if (self->v.air_finished < SV_GetTime())       SV_StartSound(self, SndChVoice, "player/gasp2.wav", VolFull, AtnNorm);
            else if (self->v.air_finished < SV_GetTime() + 9)   SV_StartSound(self, SndChVoice, "player/gasp1.wav", VolFull, AtnNorm);
            self->v.air_finished = SV_GetTime() + 12.0;
            self->v.dmg = 2;
        }

    if (!waterlevel) {
        if (flags & FL_INWATER) {
            SV_StartSound(self, SndChBody, "misc/outwater.wav", VolFull, AtnNorm);    // play leave water sound
            self->v.flags = (float)(flags & ~FL_INWATER);
        }
        self->v.air_finished = SV_GetTime() + 12.0;
        G_FLOAT(OFS_RETURN) = damage;
        return;
    }

    if (watertype == CONTENTS_LAVA) { // do damage
        if (!(flags & (FL_IMMUNE_LAVA + FL_GODMODE)))
            if (self->v.dmgtime < SV_GetTime()) {
                if (self->v.radsuit_finished < SV_GetTime())     self->v.dmgtime = SV_GetTime() + 0.2;
                else                                        self->v.dmgtime = SV_GetTime() + 1.0;
                //    T_Damage (self, world, world, 10*self.waterlevel, 0, TRUE);
                damage = (float)(10 * waterlevel);
            }
    }
    else if (watertype == CONTENTS_SLIME) { // do damage
        if (!(flags & (FL_IMMUNE_SLIME + FL_GODMODE)))
            if (self->v.dmgtime < SV_GetTime() && self->v.radsuit_finished < SV_GetTime()) {
                self->v.dmgtime = SV_GetTime() + 1.0;
                //    T_Damage (self, world, world, 4*self.waterlevel, 0, TRUE);
                damage = (float)(4 * waterlevel);
            }
    }

    if (!(flags & FL_INWATER)) {
        // player enter water sound
        if (watertype == CONTENTS_LAVA)  SV_StartSound(self, SndChBody, "player/inlava.wav", VolFull, AtnNorm);
        if (watertype == CONTENTS_WATER) SV_StartSound(self, SndChBody, "player/inh2o.wav", VolFull, AtnNorm);
        if (watertype == CONTENTS_SLIME) SV_StartSound(self, SndChBody, "player/slimbrn2.wav", VolFull, AtnNorm);

        self->v.flags = (float)(flags | FL_INWATER);
        self->v.dmgtime = 0;
    }

    if (!(flags & FL_WATERJUMP)) {
        //  self.velocity = self.velocity - 0.8*self.waterlevel*frametime*self.velocity;
        self->v.velocity = VectorMA(self->v.velocity, -0.8 * self->v.waterlevel * host_frametime, self->v.velocity);
    }

    G_FLOAT(OFS_RETURN) = damage;
}

#endif

