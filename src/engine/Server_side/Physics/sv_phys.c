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
// sv_phys.c

#include "server.h"
#include "cvar_q1.h"
#include "q_tools.h"
#include <string.h>
#include "world.h"
#include "console.h"
#include "host.h"
#include "mathlib.h"
#include "transform.h"
#include "progs.h"
#include "GlobVars.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/

/* --- movement clip flags (bitmask) --- */
typedef enum {
    MOVECLIP_NONE       = 0u,        /* no block */
    MOVECLIP_FLOOR      = 1u << 0,   /* floor (normal[Z_AX] > 0) */
    MOVECLIP_WALL       = 1u << 1,   /* wall/step (normal[Z_AX] == 0) */
    MOVECLIP_DEADSTOP   = 1u << 2,   /* dead stop (reserved by original comment) */

    /* --- special early-return results (non-bitmask) --- */
    /* note: 3 == (MOVECLIP_FLOOR|MOVECLIP_WALL) by value; here it is used as “trapped/allsolid” */
    FLYMOVE_TRAPPED = MOVECLIP_FLOOR | MOVECLIP_WALL,   /* allsolid / clipped to too many planes */
    FLYMOVE_STUCK = FLYMOVE_TRAPPED | MOVECLIP_DEADSTOP /* unresolvable geometry / still stuck */
} MoveClipFlags_e;


#define MOVE_EPSILON 0.01

void SV_Physics_Toss(edict_p ent);

/*
================
SV_CheckAllEnts
================
*/
void SV_CheckAllEnts() {
    // see if any solid entities are inside the final position
    edict_p check = ED_GetEDictFirst();
    for (int e = 1; e < EdictsNum; e++, check = ED_GetEDictNext(check)) {
        if (check->free) continue;
        if (check->v.movetype == MOVETYPE_PUSH ||
            check->v.movetype == MOVETYPE_NONE ||
#ifdef QUAKE2
            check->v.movetype == MOVETYPE_FOLLOW ||
#endif
            check->v.movetype == MOVETYPE_NOCLIP)
            continue;

        if (SV_TestEntityPosition(check))
            Con_Printf("entity in invalid position\n");
    }
}

/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity(edict_p ent) {
    //
    // bound velocity
    //
    for (int i = 0; i < VECT_DIM; i++) {
        if (IS_NAN(ent->v.velocity.v[i])) {
            Con_Printf("Got a NaN velocity on %s\n", PR_GetQString(ent->v.classname));
            ent->v.velocity.v[i] = 0.0f;
        }
        if (IS_NAN(ent->v.origin.v[i])) {
            Con_Printf("Got a NaN origin on %s\n", PR_GetQString(ent->v.classname));
            ent->v.origin.v[i] = 0.0f;
        }
        CLAMP(-sv_maxvelocity.value, ent->v.velocity.v[i], sv_maxvelocity.value);
    }
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
bool SV_RunThink(edict_p ent) {
    float thinktime = ent->v.nextthink;
    if ((thinktime <= 0) ||
        (thinktime > (SV_GetTime() + host_frametime))
        )
        return true;

    CLAMP_LESS(thinktime, (float)SV_GetTime()); // don't let things stay in the past.
    // it is possible to start that way
    // by a trigger with a local time.
    ent->v.nextthink = 0;
    pr_global_struct->time = thinktime;
    pr_global_struct->self = ED_GetEDictOffs(ent);
    pr_global_struct->other = ED_GetEDictOffs(Edicts); // should be 0
    PR_ExecuteProgram(ent->v.think);
    return !ent->free;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact(edict_p e1, edict_p e2) {
    int old_self = pr_global_struct->self;
    int old_other = pr_global_struct->other;

    pr_global_struct->time = (float)SV_GetTime();
    if (e1->v.touch && (e1->v.solid != SOLID_NOT)) {
        pr_global_struct->self = ED_GetEDictOffs(e1);
        pr_global_struct->other = ED_GetEDictOffs(e2);
        PR_ExecuteProgram(e1->v.touch);
    }

    if (e2->v.touch && e2->v.solid != SOLID_NOT) {
        pr_global_struct->self = ED_GetEDictOffs(e2);
        pr_global_struct->other = ED_GetEDictOffs(e1);
        PR_ExecuteProgram(e2->v.touch);
    }

    pr_global_struct->self = old_self;
    pr_global_struct->other = old_other;
}


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define STOP_EPSILON 0.1

MoveClipFlags_e ClipVelocity(vec3_t in, vec3_t normal, vec3_p out, float overbounce) {
    MoveClipFlags_e blocked = MOVECLIP_NONE;
    if (normal.z > 0.0f)    blocked |= MOVECLIP_FLOOR;  // floor
    if (!(normal.z))        blocked |= MOVECLIP_WALL;  // step

    float backoff = DotProduct(in, normal) * overbounce;

    *out = VectorMA(in, -backoff, normal);
    for (int i = 0; i < VECT_DIM; i++) {
        if ((out->v[i] > -STOP_EPSILON) &&
            (out->v[i] < STOP_EPSILON)
            )
            out->v[i] = 0.0f;
    }

    return blocked;
}


/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
#define MAX_CLIP_PLANES 5
MoveClipFlags_e SV_FlyMove(edict_p ent, float time, trace_p steptrace) {
    MoveClipFlags_e blocked = MOVECLIP_NONE;
    vec3_t original_velocity = ent->v.velocity;
    vec3_t primal_velocity = ent->v.velocity;
    int numplanes = 0;

    float time_left = time;

    int numbumps = 4;
    vec3_t planes[MAX_CLIP_PLANES];
    for (int bumpcount = 0; bumpcount < numbumps; bumpcount++) {
        if (!(ent->v.velocity.x) &&
            !(ent->v.velocity.y) &&
            !(ent->v.velocity.z)) break; // ent->v.velocity.is_zero()

        vec3_t end = VectorMA(ent->v.origin, time_left, ent->v.velocity);

        trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);

        if (trace.allsolid) { // entity is trapped in another solid
            ent->v.velocity = vec3_origin;
            return FLYMOVE_TRAPPED;
        }

        if (trace.fraction > 0) { // actually covered some distance
            ent->v.origin = trace.endpos;
            original_velocity = ent->v.velocity;
            numplanes = 0;
        }

        if (trace.fraction == 1)    break;  // moved the entire distance
        if (!trace.ent)             Host_SysError("SV_FlyMove: !trace.ent");

        if (trace.plane.normal.z > 0.7) {
            blocked |= MOVECLIP_FLOOR;  // floor
            if (trace.ent->v.solid == SOLID_BSP) {
                ent->v.flags = (float)((EntityFlags_t)ent->v.flags | FL_ONGROUND);
                ent->v.groundentity = ED_GetEDictOffs(trace.ent);
            }
        }
        if (!trace.plane.normal.z) {
            blocked |= MOVECLIP_WALL;  // step
            if (steptrace)
                *steptrace = trace; // save for player extrafriction
        }

        //
        // run the impact function
        //
        SV_Impact(ent, trace.ent);
        if (ent->free)  break;  // removed by the impact function


        time_left -= time_left * trace.fraction;

        // cliped to another plane
        if (numplanes >= MAX_CLIP_PLANES) { // this shouldn't really happen
            ent->v.velocity = vec3_origin;
            return FLYMOVE_TRAPPED;
        }

        planes[numplanes] = trace.plane.normal;
        numplanes++;

        //
        // modify original_velocity so it parallels all of the clip planes
        //
        {
            vec3_t  new_velocity;
            int i = 0;
            for (; i < numplanes; i++) {
                ClipVelocity(original_velocity, planes[i], &new_velocity, 1);
                int j = 0;
                for (; j < numplanes; j++)
                    if ((j != i) &&
                        (DotProduct(new_velocity, planes[j]) < 0))
                        break; // not ok

                if (j == numplanes)     break;
            }

            if (i != numplanes) { // go along this plane
                ent->v.velocity = new_velocity;
            }
            else { // go along the crease
                if (numplanes != 2) {
                    //    Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
                    ent->v.velocity = vec3_origin;
                    return FLYMOVE_STUCK;
                }
                vec3_t dir = CrossProduct(planes[X_AX], planes[Y_AX]);
                float d = DotProduct(dir, ent->v.velocity);
                ent->v.velocity = VectorScale(dir, d);
            }
        }

        //
        // if original velocity is against the original velocity, stop dead
        // to avoid tiny occilations in sloping corners
        //
        if (DotProduct(ent->v.velocity, primal_velocity) <= 0) {
            ent->v.velocity = vec3_origin;
            return blocked;
        }
    }

    return blocked;
}


/*
============
SV_AddGravity

============
*/
void SV_AddGravity(edict_p ent) {
#ifdef QUAKE2
    float ent_gravity = (ent->v.gravity) ? ent->v.gravity : 1.0f;
#else
    eval_p val = GetEdictFieldValue(ent, "gravity");
    float ent_gravity = (val && val->_float) ? val->_float : 1.0f;
#endif
    ent->v.velocity.z -= (float)(ent_gravity * sv_gravity.value * host_frametime);
}


/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
trace_t SV_PushEntity(edict_p ent, vec3_t push) {
    vec3_t end = VectorAdd(ent->v.origin, push);

    trace_t trace;
    if (ent->v.movetype == MOVETYPE_FLYMISSILE)     trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_MISSILE, ent);
    else
        switch ((solid_t)ent->v.solid) {
        case SOLID_TRIGGER:  // only clip against bmodels
        case SOLID_NOT:                             trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NOMONSTERS, ent);    break;
        default:                                    trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);        break;
        }
    ent->v.origin = trace.endpos;
    SV_LinkEdict(ent, true);

    if (trace.ent)
        SV_Impact(ent, trace.ent);

    return trace;
}


/*
============
SV_PushMove

============
*/
void SV_PushMove(edict_p pusher, float movetime) {
    if (!(pusher->v.velocity.x) &&
        !(pusher->v.velocity.y) &&
        !(pusher->v.velocity.z)
        ) {
        pusher->v.ltime += movetime;
        return;
    }

    vec3_t move = VectorScale(pusher->v.velocity, movetime);
    vec3_t mins = VectorAdd(pusher->v.absmin, move);
    vec3_t maxs = VectorAdd(pusher->v.absmax, move);

    vec3_t pushorig = pusher->v.origin;

    // move the pusher to it's final position
    pusher->v.origin = VectorAdd(pusher->v.origin, move);
    pusher->v.ltime += movetime;
    SV_LinkEdict(pusher, false);

    // see if any solid entities are inside the final position
    int num_moved = 0;
    edict_p check = ED_GetEDictFirst();
    edict_p moved_edict[MAX_EDICTS];
    vec3_t  moved_from[MAX_EDICTS];
    for (int e = 1; e < EdictsNum; e++, check = ED_GetEDictNext(check)) {
        if (check->free)    continue;

        switch ((movetype_t)check->v.movetype) {
#ifdef QUAKE2
        case MOVETYPE_FOLLOW:
#endif
        case MOVETYPE_PUSH:
        case MOVETYPE_NONE:
        case MOVETYPE_NOCLIP:   continue;

        default:                break;
        }

        // if the entity is standing on the pusher, it will definately be moved
        if (!(((EntityFlags_t)check->v.flags & FL_ONGROUND) &&
            ED_GetEDictByOffs(check->v.groundentity) == pusher)) {
            if (
                (
                    check->v.absmin.x >= maxs.x ||
                    check->v.absmin.y >= maxs.y ||
                    check->v.absmin.z >= maxs.z) ||
                (
                    check->v.absmax.x <= mins.x ||
                    check->v.absmax.y <= mins.y ||
                    check->v.absmax.z <= mins.z)
                )
                continue;

            // see if the ent's bbox is inside the pusher's final position
            if (!SV_TestEntityPosition(check))
                continue;
        }

        // remove the onground flag for non-players
        if (check->v.movetype != MOVETYPE_WALK)
            check->v.flags = (float)((int)((EntityFlags_t)check->v.flags) & ~FL_ONGROUND);

        vec3_t entorig = check->v.origin;
        moved_from[num_moved] = check->v.origin;
        moved_edict[num_moved] = check;
        num_moved++;

        // try moving the contacted entity
        pusher->v.solid = SOLID_NOT;
        SV_PushEntity(check, move);
        pusher->v.solid = SOLID_BSP;

        // if it is still inside the pusher, block
        edict_p block = SV_TestEntityPosition(check);
        if (block) { // fail the move
            if (check->v.mins.x == check->v.maxs.x) continue;

            if (check->v.solid == SOLID_NOT ||
                check->v.solid == SOLID_TRIGGER) { // corpse
                check->v.mins.x = check->v.mins.y = 0;
                check->v.maxs = check->v.mins;
                continue;
            }

            check->v.origin = entorig;
            SV_LinkEdict(check, true);

            pusher->v.origin = pushorig;
            SV_LinkEdict(pusher, false);
            pusher->v.ltime -= movetime;

            // if the pusher has a "blocked" function, call it
            // otherwise, just stay in place until the obstacle is gone
            if (pusher->v.blocked) {
                pr_global_struct->self = ED_GetEDictOffs(pusher);
                pr_global_struct->other = ED_GetEDictOffs(check);
                PR_ExecuteProgram(pusher->v.blocked);
            }

            // move back any entities we already moved
            for (int i = 0; i < num_moved; i++) {
                moved_edict[i]->v.origin = moved_from[i];
                SV_LinkEdict(moved_edict[i], false);
            }
            return;
        }
    }


}

#ifdef QUAKE2
/*
============
SV_PushRotate

============
*/
void SV_PushRotate(edict_p pusher, float movetime) {
    if (!(pusher->v.avelocity.x) &&
        !(pusher->v.avelocity.y) &&
        !(pusher->v.avelocity.z)) {
        pusher->v.ltime += movetime;
        return;
    }


    vec3_t amove = VectorScale(pusher->v.avelocity, movetime);

    vec3_t a = VectorSubtract(vec3_origin, amove);
    vec3_t forward, right, up;  AngleVectors(a, forward, right, up);

    vec3_t pushorig = pusher->v.angles;

    // move the pusher to it's final position
    pusher->v.angles = VectorAdd(pusher->v.angles, amove);
    pusher->v.ltime += movetime;
    SV_LinkEdict(pusher, false);


    // see if any solid entities are inside the final position
    int num_moved = 0;
    edict_p check = ED_GetEDictFirst();
    edict_p moved_edict[MAX_EDICTS];
    vec3_t  moved_from[MAX_EDICTS];
    for (int e = 1; e < EdictsNum; e++, check = ED_GetEDictNext(check)) {
        if (check->free)    continue;

        if (check->v.movetype == MOVETYPE_PUSH ||
            check->v.movetype == MOVETYPE_NONE ||
            check->v.movetype == MOVETYPE_FOLLOW ||
            check->v.movetype == MOVETYPE_NOCLIP)
            continue;

        // if the entity is standing on the pusher, it will definately be moved
        if (!(((EntityFlags_t)check->v.flags & FL_ONGROUND) &&
            ED_GetEDictByOffs(check->v.groundentity) == pusher)) {
            if (
                (
                    check->v.absmin.x >= pusher->v.absmax.x ||
                    check->v.absmin.y >= pusher->v.absmax.y ||
                    check->v.absmin.z >= pusher->v.absmax.z) ||
                (
                    check->v.absmax.x <= pusher->v.absmin.x ||
                    check->v.absmax.y <= pusher->v.absmin.y ||
                    check->v.absmax.z <= pusher->v.absmin.z)
                )
                continue;

            // see if the ent's bbox is inside the pusher's final position
            if (!SV_TestEntityPosition(check))
                continue;
        }

        // remove the onground flag for non-players
        if (check->v.movetype != MOVETYPE_WALK)
            check->v.flags = (EntityFlags_t)check->v.flags & ~FL_ONGROUND;

        vec3_t  entorig = check->v.origin;
        moved_from[num_moved] = check->v.origin;
        moved_edict[num_moved] = check;
        num_moved++;

        // calculate destination position
        vec3_t org = VectorSubtract(check->v.origin, pusher->v.origin);
        vec3_t org2 = {
            DotProduct(org, forward),
            -DotProduct(org, right),
            DotProduct(org, up)
        };
        vec3_t move = VectorSubtract(org2, org);

        // try moving the contacted entity
        pusher->v.solid = SOLID_NOT;
        SV_PushEntity(check, move);
        pusher->v.solid = SOLID_BSP;

        // if it is still inside the pusher, block
        edict_p block = SV_TestEntityPosition(check);
        if (block) { // fail the move
            if (check->v.mins.x == check->v.maxs.x)   continue;

            if ((check->v.solid == SOLID_NOT) ||
                (check->v.solid == SOLID_TRIGGER)) { // corpse
                check->v.mins.x = check->v.mins.y = 0;
                check->v.maxs = check->v.mins;
                continue;
            }

            check->v.origin = entorig;
            SV_LinkEdict(check, true);

            pusher->v.angles = pushorig;
            SV_LinkEdict(pusher, false);
            pusher->v.ltime -= movetime;

            // if the pusher has a "blocked" function, call it
            // otherwise, just stay in place until the obstacle is gone
            if (pusher->v.blocked) {
                pr_global_struct->self = ED_GetEDictOffs(pusher);
                pr_global_struct->other = ED_GetEDictOffs(check);
                PR_ExecuteProgram(pusher->v.blocked);
            }

            // move back any entities we already moved
            for (int i = 0; i < num_moved; i++) {
                moved_edict[i]->v.origin = moved_from[i];
                moved_edict[i]->v.angles = VectorSubtract(moved_edict[i]->v.angles, amove);
                SV_LinkEdict(moved_edict[i], false);
            }
            return;
        }
        else {
            check->v.angles = VectorAdd(check->v.angles, amove);
        }
    }


}
#endif

/*
================
SV_Physics_Pusher

================
*/
void SV_Physics_Pusher(edict_p ent) {
    float oldltime = ent->v.ltime;

    float thinktime = ent->v.nextthink;
    float movetime;
    if (thinktime < ent->v.ltime + host_frametime) {
        movetime = thinktime - ent->v.ltime;
        if (movetime < 0)
            movetime = 0;
    }
    else
        movetime = (float)host_frametime;

    if (movetime) {
#ifdef QUAKE2
        if (ent->v.avelocity[X_AX] ||
            ent->v.avelocity[Y_AX] ||
            ent->v.avelocity[Z_AX])
            SV_PushRotate(ent, movetime);
        else
#endif
            SV_PushMove(ent, movetime); // advances ent->v.ltime if not blocked
    }

    if ((thinktime > oldltime) &&
        (thinktime <= ent->v.ltime)
        ) {
        ent->v.nextthink = 0;
        pr_global_struct->time = (float)SV_GetTime();
        pr_global_struct->self = ED_GetEDictOffs(ent);
        pr_global_struct->other = ED_GetEDictOffs(Edicts); // should be 0
        PR_ExecuteProgram(ent->v.think);
        if (ent->free)
            return;
    }

}


/*
===============================================================================

CLIENT MOVEMENT

===============================================================================
*/

/*
=============
SV_CheckStuck

This is a big hack to try and fix the rare case of getting stuck in the world
clipping hull.
=============
*/
void SV_CheckStuck(edict_p ent) {

    if (!SV_TestEntityPosition(ent)) {
        ent->v.oldorigin = ent->v.origin;
        return;
    }

    vec3_t org = ent->v.origin;
    ent->v.origin = ent->v.oldorigin;
    if (!SV_TestEntityPosition(ent)) {
        Con_DPrintf("Unstuck.\n");
        SV_LinkEdict(ent, true);
        return;
    }

    for (int z = 0; z < 18; z++)
        for (int i = -1; i <= 1; i++)
            for (int j = -1; j <= 1; j++) {
                ent->v.origin.x = org.x + (float)i;
                ent->v.origin.y = org.y + (float)j;
                ent->v.velocity.z = org.z + (float)z;
                if (!SV_TestEntityPosition(ent)) {
                    Con_DPrintf("Unstuck.\n");
                    SV_LinkEdict(ent, true);
                    return;
                }
            }

    ent->v.origin = org;
    Con_DPrintf("player is stuck.\n");
}


/*
=============
SV_CheckWater
=============
*/
bool SV_CheckWater(edict_p ent) {
    vec3_t point = {
        .x = ent->v.origin.x,
        .y = ent->v.origin.y,
        .z = ent->v.velocity.z + ent->v.mins.z + 1
    };

    ent->v.waterlevel = 0;
    ent->v.watertype = CONTENTS_EMPTY;
    contents_t cont = SV_PointContents(point);
    if (cont <= CONTENTS_WATER) {
#ifdef QUAKE2
        contents_t truecont = SV_TruePointContents(point);
#endif
        ent->v.watertype = cont;
        ent->v.waterlevel = 1;
        point.z = ent->v.origin.z + (ent->v.mins.z + ent->v.maxs.z) * 0.5f;
        cont = SV_PointContents(point);
        if (cont <= CONTENTS_WATER) {
            ent->v.waterlevel = 2;
            point.z = ent->v.origin.z + ent->v.view_ofs.z;
            cont = SV_PointContents(point);
            if (cont <= CONTENTS_WATER)
                ent->v.waterlevel = 3;
        }
#ifdef QUAKE2
        if ((truecont <= CONTENTS_CURRENT_0) &&
            (truecont >= CONTENTS_CURRENT_DOWN)) {
            static vec3_t current_table[] =
            {
                {1, 0, 0},
                {0, 1, 0},
                {-1, 0, 0},
                {0, -1, 0},
                {0, 0, 1},
                {0, 0, -1}
            };

            ent->v.basevelocity = VectorMA(
                ent->v.basevelocity,
                150.0 * ent->v.waterlevel / 3.0,
                current_table[CONTENTS_CURRENT_0 - truecont]
            );
        }
#endif
    }

    return ent->v.waterlevel > 1;
}

/*
============
SV_WallFriction

============
*/
void SV_WallFriction(edict_p ent, trace_p trace) {
    Basis_t bs = GetBasis(ent->v.v_angle);

    float d = DotProduct(trace->plane.normal, bs.forward);

    d += 0.5f;
    if (d >= 0)
        return;

    // cut the tangential velocity
    float i = DotProduct(trace->plane.normal, ent->v.velocity);
    vec3_t into = VectorScale(trace->plane.normal, i);
    vec3_t side = VectorSubtract(ent->v.velocity, into);

    ent->v.velocity.x = side.x * (1 + d);
    ent->v.velocity.y = side.y * (1 + d);
}

/*
=====================
SV_TryUnstick

Player has come to a dead stop, possibly due to the problem with limited
float precision at some angle joins in the BSP hull.

Try fixing by pushing one pixel in each direction.

This is a hack, but in the interest of good gameplay...
======================
*/
MoveClipFlags_e SV_TryUnstick(edict_p ent, vec3_t oldvel) {
    vec3_t oldorg = ent->v.origin;
    vec3_t dir = vec3_origin;

    for (int i = 0; i < 8; i++) {
        // try pushing a little in an axial direction
        switch (i) {
        case 0:     dir.x = 2;  dir.y = 0;  break;
        case 1:     dir.x = 0;  dir.y = 2;  break;
        case 2:     dir.x = -2; dir.y = 0;  break;
        case 3:     dir.x = 0;  dir.y = -2; break;
        case 4:     dir.x = 2;  dir.y = 2;  break;
        case 5:     dir.x = -2; dir.y = 2;  break;
        case 6:     dir.x = 2;  dir.y = -2; break;
        case 7:     dir.x = -2; dir.y = -2; break;
        }

        SV_PushEntity(ent, dir);

        // retry the original move
        ent->v.velocity.x = oldvel.x;
        ent->v.velocity.y = oldvel.y;
        ent->v.velocity.z = 0;
        trace_t steptrace;
        MoveClipFlags_e clip = SV_FlyMove(ent, 0.1f, &steptrace);

        if ((fabs(oldorg.y - ent->v.origin.y) > 4) ||
            (fabs(oldorg.x - ent->v.origin.x) > 4)) {
            //Con_DPrintf ("unstuck!\n");
            return clip;
        }

        // go back to the original pos and try again
        ent->v.origin = oldorg;
    }

    ent->v.velocity = vec3_origin;
    return FLYMOVE_STUCK;  // still not moving
}

/*
=====================
SV_WalkMove

Only used by players
======================
*/
#define STEPSIZE 18
void SV_WalkMove(edict_p ent) {
    // do a regular slide move unless it looks like you ran into a step
    bool oldonground = (EntityFlags_t)ent->v.flags & FL_ONGROUND;
    ent->v.flags = (float)((int)((EntityFlags_t)ent->v.flags) & ~FL_ONGROUND);

    vec3_t oldorg = ent->v.origin;
    vec3_t oldvel = ent->v.velocity;

    trace_t steptrace;

    if (
        !(SV_FlyMove(ent, (float)host_frametime, &steptrace) & MOVECLIP_WALL) ||   // move didn't block on a step
        (!oldonground && (ent->v.waterlevel == 0)) ||                       // don't stair up while jumping
        ((movetype_t)ent->v.movetype != MOVETYPE_WALK) ||                   // gibbed by a trigger
        sv_nostep.value ||                                                  // no stepping allowed
        ((EntityFlags_t)sv_player->v.flags & FL_WATERJUMP)                  // waterjump active
        ) {
        return;
    }

    vec3_t nosteporg = ent->v.origin;
    vec3_t nostepvel = ent->v.velocity;

    // try moving up and forward to go up a step
    ent->v.origin = oldorg; // back to start pos

    vec3_t upmove = vec3_origin;
    upmove.z = STEPSIZE;
    vec3_t downmove = vec3_origin;
    downmove.z = (float)(-STEPSIZE + oldvel.z * host_frametime);

    // move up
    SV_PushEntity(ent, upmove); // FIXME: don't link?

    // move forward
    ent->v.velocity.x = oldvel.x;
    ent->v.velocity.y = oldvel.y;
    ent->v.velocity.z = 0.0f;
    MoveClipFlags_e clip = SV_FlyMove(ent, (float)host_frametime, &steptrace);

    // check for stuckness, possibly due to the limited precision of floats
    // in the clipping hulls
    if (clip) {
        if ((fabs(oldorg.y - ent->v.origin.y) < 0.03125) &&
            (fabs(oldorg.x - ent->v.origin.x) < 0.03125)    // stepping up didn't make any progress
            ) {
            clip = SV_TryUnstick(ent, oldvel);
        }
    }

    // extra friction based on view angle
    if (clip & MOVECLIP_WALL)   SV_WallFriction(ent, &steptrace);

    // move down
    trace_t downtrace = SV_PushEntity(ent, downmove); // FIXME: don't link?

    if (downtrace.plane.normal.z > 0.7) {
        if (ent->v.solid == SOLID_BSP) {
            ent->v.flags = (float)((int)((EntityFlags_t)ent->v.flags) | FL_ONGROUND);
            ent->v.groundentity = ED_GetEDictOffs(downtrace.ent);
        }
    }
    else {
        // if the push down didn't end up on good ground, use the move without
        // the step up.  This happens near wall / slope combinations, and can
        // cause the player to hop up higher on a slope too steep to climb
        ent->v.origin = nosteporg;
        ent->v.velocity = nostepvel;
    }
}


/*
================
SV_Physics_Client

Player character actions
================
*/
void SV_Physics_Client(edict_p ent, int num) {
    if (!svs.clients[num - 1].active)   return;  // unconnected slot

    //
    // call standard client pre-think
    //
    pr_global_struct->time = (float)SV_GetTime();
    pr_global_struct->self = ED_GetEDictOffs(ent);
    PR_ExecuteProgram(pr_global_struct->PlayerPreThink);

    //
    // do a move
    //
    SV_CheckVelocity(ent);

    //
    // decide which move function to call
    //
    // first: entities that require thinking before physics
    switch ((movetype_t)ent->v.movetype) {
    case MOVETYPE_NONE:
    case MOVETYPE_WALK:
    case MOVETYPE_FLY:
    case MOVETYPE_NOCLIP:        if (!SV_RunThink(ent)) return;

    default:    break;  // other movetypes handled later
    }

    // second: actual physics behavior
    switch ((movetype_t)ent->v.movetype) {
    case MOVETYPE_NONE:     break;  // no movement, just think
    case MOVETYPE_WALK:
        if (!SV_CheckWater(ent) &&
            !((EntityFlags_t)ent->v.flags & FL_WATERJUMP)
            )
            SV_AddGravity(ent);

        SV_CheckStuck(ent);
#ifdef QUAKE2
        ent->v.velocity = VectorAdd(ent->v.velocity, ent->v.basevelocity); // ent->v.velocity += ent->v.basevelocity;
        SV_WalkMove(ent);
        ent->v.velocity = VectorSubtract(ent->v.velocity, ent->v.basevelocity); // ent->v.velocity -= ent->v.basevelocity;
#else
        SV_WalkMove(ent);
#endif
        break;

    case MOVETYPE_TOSS:
    case MOVETYPE_BOUNCE:   SV_Physics_Toss(ent);                   break;
    case MOVETYPE_FLY:      SV_FlyMove(ent, (float)host_frametime, NULL);  break;
    case MOVETYPE_NOCLIP:   ent->v.origin = VectorMA(ent->v.origin, (float)host_frametime, ent->v.velocity);    break;
    default:                Host_SysError("SV_Physics_Client: bad movetype %i", (int)ent->v.movetype);
    }

    //
    // call standard player post-think
    //
    SV_LinkEdict(ent, true);

    pr_global_struct->time = (float)SV_GetTime();
    pr_global_struct->self = ED_GetEDictOffs(ent);
    PR_ExecuteProgram(pr_global_struct->PlayerPostThink);
}

//============================================================================

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None(edict_p ent) { SV_RunThink(ent); } // regular thinking

#ifdef QUAKE2
/*
=============
SV_Physics_Follow

Entities that are "stuck" to another entity
=============
*/
void SV_Physics_Follow(edict_p ent) {
    SV_RunThink(ent);    // regular thinking
    ent->v.origin = VectorAdd(ED_GetEDictByOffs(ent->v.aiment)->v.origin, ent->v.v_angle);
    SV_LinkEdict(ent, true);
}
#endif

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip(edict_p ent) {
    // regular thinking
    if (!SV_RunThink(ent))  return;

    ent->v.angles = VectorMA(ent->v.angles, (float)host_frametime, ent->v.avelocity);
    ent->v.origin = VectorMA(ent->v.origin, (float)host_frametime, ent->v.velocity);

    SV_LinkEdict(ent, false);
}

/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition(edict_p ent) {
#ifdef QUAKE2
    vec3_t point = {
        ent->v.origin[X_AX],
        ent->v.origin[Y_AX],
        ent->v.velocity[Z_AX] + ent->v.mins[Z_AX] + 1
    };
    int cont = SV_PointContents(point);
#else
    int cont = SV_PointContents(ent->v.origin);
#endif
    if (!ent->v.watertype) { // just spawned here
        ent->v.watertype = (float)cont;
        ent->v.waterlevel = 1;
        return;
    }

    if ((contents_t)ent->v.watertype == CONTENTS_EMPTY) // just crossed into water
        SV_StartSound(ent, 0, "misc/h2ohit1.wav", 255, 1);

    if (cont <= CONTENTS_WATER) {
        ent->v.watertype = (float)cont;
        ent->v.waterlevel = 1;
    }
    else {
        ent->v.watertype = CONTENTS_EMPTY;
        ent->v.waterlevel = (float)cont;
    }
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
#ifdef QUAKE2
static vec3_t _vecOrigin = {
    .x = 0.0f,
    .y = 0.0f,
    .z = 0.0f
};
#endif
void SV_Physics_Toss(edict_p ent) {
#ifdef QUAKE2
    edict_p groundentity = ED_GetEDictByOffs(ent->v.groundentity);
    if ((EntityFlags_t)groundentity->v.flags & FL_CONVEYOR) ent->v.basevelocity = VectorScale(groundentity->v.movedir, groundentity->v.speed);
    else                                                    ent->v.basevelocity = _vecOrigin;
    SV_CheckWater(ent);
#endif
    // regular thinking
    if (!SV_RunThink(ent))  return;

#ifdef QUAKE2
    if (ent->v.velocity[Z_AX] > 0)
        ent->v.flags = (EntityFlags_t)ent->v.flags & ~FL_ONGROUND;

    if ((((EntityFlags_t)ent->v.flags & FL_ONGROUND)) &&
        (VectorCompare(ent->v.basevelocity, _vecOrigin)))
        return;

    SV_CheckVelocity(ent);

    // add gravity
    if (!((EntityFlags_t)ent->v.flags & FL_ONGROUND) &&
        ent->v.movetype != MOVETYPE_FLY &&
        ent->v.movetype != MOVETYPE_BOUNCEMISSILE &&
        ent->v.movetype != MOVETYPE_FLYMISSILE)
        SV_AddGravity(ent);

#else
    // if onground, return without moving
    if (((EntityFlags_t)ent->v.flags & FL_ONGROUND))  return;

    SV_CheckVelocity(ent);

    // add gravity
    switch ((movetype_t)ent->v.movetype) {
    case MOVETYPE_FLY:
    case MOVETYPE_FLYMISSILE:   /* no gravity */    break;
    default:                    SV_AddGravity(ent); break;
    }
#endif

    // move angles
    ent->v.angles = VectorMA(ent->v.angles, (float)host_frametime, ent->v.avelocity);

    // move origin
#ifdef QUAKE2
    ent->v.velocity = VectorAdd(ent->v.velocity, ent->v.basevelocity);
#endif
    vec3_t move = VectorScale(ent->v.velocity, (float)host_frametime);
    trace_t trace = SV_PushEntity(ent, move);
#ifdef QUAKE2
    ent->v.velocity = VectorSubtract(ent->v.velocity, ent->v.basevelocity);
#endif
    if ((trace.fraction == 1) ||
        (ent->free))
        return;

    float backoff;
    switch ((movetype_t)ent->v.movetype) {
#ifdef QUAKE2
    case MOVETYPE_BOUNCEMISSILE:    backoff = 2.0;  break;
#endif
    case MOVETYPE_BOUNCE:           backoff = 1.5;  break;
    default:                        backoff = 1.0;  break;
    }

    ClipVelocity(ent->v.velocity, trace.plane.normal, &ent->v.velocity, backoff);

    // stop if on ground
    if ((trace.plane.normal.z > 0.7) &&
        ((ent->v.velocity.z < 60) ||
            (
                (ent->v.movetype != MOVETYPE_BOUNCE)
#ifdef QUAKE2
                && (ent->v.movetype != MOVETYPE_BOUNCEMISSILE)
#endif
                )
            )) {
        ent->v.flags = (float)((int)((EntityFlags_t)ent->v.flags) | FL_ONGROUND);
        ent->v.groundentity = ED_GetEDictOffs(trace.ent);
        ent->v.velocity = vec3_origin;
        ent->v.avelocity = vec3_origin;

    }

    // check for in water
    SV_CheckWaterTransition(ent);
}

/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
=============
*/
#ifdef QUAKE2
void SV_Physics_Step(edict_p ent) {
    bool hitsound = false;

    edict_p groundentity = ED_GetEDictByOffs(ent->v.groundentity);
    if ((EntityFlags_t)groundentity->v.flags & FL_CONVEYOR)     ent->v.basevelocity = VectorScale(groundentity->v.movedir, groundentity->v.speed);
    else                                                        ent->v.basevelocity = _vecOrigin;
    //@@
    pr_global_struct->time = SV_GetTime();
    pr_global_struct->self = ED_GetEDictOffs(ent);
    PF_WaterMove();

    SV_CheckVelocity(ent);

    bool wasonground = (EntityFlags_t)ent->v.flags & FL_ONGROUND;
    // ent->v.flags = (EntityFlags_t)ent->v.flags & ~FL_ONGROUND;

        // add gravity except:
        //   flying monsters
        //   swimming monsters who are in the water
    bool inwater = SV_CheckWater(ent);
    if ((!wasonground) &&
        (!((EntityFlags_t)ent->v.flags & FL_FLY)) &&
        (!(
            ((EntityFlags_t)ent->v.flags & FL_SWIM) &&
            (ent->v.waterlevel > 0)
            ))) {
        hitsound = (ent->v.velocity[Z_AX] < (sv_gravity.value * -0.1));
        if (!inwater)   SV_AddGravity(ent);
    }

    if (!VectorCompare(ent->v.velocity, _vecOrigin) ||
        !VectorCompare(ent->v.basevelocity, _vecOrigin)) {
        ent->v.flags = (EntityFlags_t)ent->v.flags & ~FL_ONGROUND;
        // apply friction
        // let dead monsters who aren't completely onground slide
        if (wasonground)
            if (!((ent->v.health <= 0.0) &&
                !SV_CheckBottom(ent))) {
                vec3_p vel = ent->v.velocity;
                float speed = sqrt((vel->x * vel->x) + (vel->y * vel->y));
                if (speed) {
                    float friction = sv_friction.value;

                    float control = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
                    float newspeed = speed - host_frametime * control * friction;

                    if (newspeed < 0)
                        newspeed = 0;
                    newspeed /= speed;

                    vel->x = vel->x * newspeed;
                    vel->y = vel->y * newspeed;
                }
            }

        VectorAdd(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);
        SV_FlyMove(ent, host_frametime, NULL);
        ent->v.velocity = VectorSubtract(ent->v.velocity, ent->v.basevelocity);

        // determine if it's on solid ground at all
        {
            vec3_t mins = VectorAdd(ent->v.origin, ent->v.mins);
            vec3_t maxs = VectorAdd(ent->v.origin, ent->v.maxs);

            vec3_t point;
            point[Z_AX] = mins[Z_AX] - 1;
            for (int x = 0; x <= 1; x++)
                for (int y = 0; y <= 1; y++) {
                    point[X_AX] = x ? maxs[X_AX] : mins[X_AX];
                    point[Y_AX] = y ? maxs[Y_AX] : mins[Y_AX];
                    if (SV_PointContents(point) == CONTENTS_SOLID) {
                        ent->v.flags = (EntityFlags_t)ent->v.flags | FL_ONGROUND;
                        break;
                    }
                }

        }

        SV_LinkEdict(ent, true);

        if (((EntityFlags_t)ent->v.flags & FL_ONGROUND) &&
            (!wasonground) &&
            (hitsound))
            SV_StartSound(ent, 0, "demon/dland2.wav", 255, 1);
    }

    // regular thinking
    SV_RunThink(ent);
    SV_CheckWaterTransition(ent);
}
#else
void SV_Physics_Step(edict_p ent) {
    // freefall if not onground
    if (!((EntityFlags_t)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM))) {
        bool hitsound = (ent->v.velocity.z < sv_gravity.value * -0.1);

        SV_AddGravity(ent);
        SV_CheckVelocity(ent);
        SV_FlyMove(ent, (float)host_frametime, NULL);
        SV_LinkEdict(ent, true);

        if (((EntityFlags_t)ent->v.flags & FL_ONGROUND) && // just hit ground
            (hitsound))
            SV_StartSound(ent, 0, "demon/dland2.wav", 255, 1);
    }

    // regular thinking
    SV_RunThink(ent);

    SV_CheckWaterTransition(ent);
}
#endif

//============================================================================

/*
================
SV_Physics

================
*/
void SV_Physics() {
    // let the progs know that a new frame has started
    pr_global_struct->self = ED_GetEDictOffs(Edicts); // should be 0
    pr_global_struct->other = ED_GetEDictOffs(Edicts); // should be 0
    pr_global_struct->time = (float)SV_GetTime();
    PR_ExecuteProgram(pr_global_struct->StartFrame);

    //SV_CheckAllEnts();

    //
    // treat each object in turn
    edict_p ent = Edicts;
    for (int i = 0; i < EdictsNum; i++, ent = ED_GetEDictNext(ent)) {
        if (ent->free)  continue;

        if (pr_global_struct->force_retouch) {
            SV_LinkEdict(ent, true); // force retouch even for stationary
        }

        if ((i > 0) && (i <= svs.maxClients)) {
            SV_Physics_Client(ent, i);
        }
        else {
            switch ((movetype_t)ent->v.movetype) {
#ifdef QUAKE2
            case MOVETYPE_FOLLOW:       SV_Physics_Follow(ent); break;
            case MOVETYPE_BOUNCEMISSILE:
#endif
            case MOVETYPE_TOSS:
            case MOVETYPE_BOUNCE:
            case MOVETYPE_FLY:
            case MOVETYPE_FLYMISSILE:   SV_Physics_Toss(ent);   break;
            case MOVETYPE_PUSH:         SV_Physics_Pusher(ent); break;
            case MOVETYPE_NONE:         SV_Physics_None(ent);   break;
            case MOVETYPE_NOCLIP:       SV_Physics_Noclip(ent); break;
            case MOVETYPE_STEP:         SV_Physics_Step(ent);   break;

            default:    Host_SysError("SV_Physics: bad movetype %i", (movetype_t)ent->v.movetype); break;
            }
        }
    }

    if (pr_global_struct->force_retouch)
        pr_global_struct->force_retouch--;

    sv.time += host_frametime;
}


#ifndef QUAKE2
trace_t SV_Trace_Toss(edict_p ent, edict_p ignore) {
    LegTime_t save_frametime = host_frametime;
    host_frametime = 0.05;

    edict_t tempent; memcpy(&tempent, ent, sizeof(edict_t));
    edict_p tent = &tempent;

    while (1) {
        SV_CheckVelocity(tent);
        SV_AddGravity(tent);
        tent->v.angles = VectorMA(tent->v.angles, (float)host_frametime, tent->v.avelocity);
        vec3_t move = VectorScale(tent->v.velocity, (float)host_frametime);
        vec3_t end = VectorAdd(tent->v.origin, move);
        trace_t trace = SV_Move(tent->v.origin, tent->v.mins, tent->v.maxs, end, MOVE_NORMAL, tent);
        tent->v.origin = trace.endpos;

# if 0
        exter_n Particle_p active_particles, free_particles;
        Particle_p p = free_particles;
        if (p) {
            free_particles = p->next;
            p->next = active_particles;
            active_particles = p;

            p->die = 256;
            p->color = 15;
            p->type = pt_static;
            p->vel = vec3_origin;
            p->org = tent->v.origin;
        }
# endif

        if ((trace.ent) &&
            (trace.ent != ignore))
            // p->color = 224;
            host_frametime = save_frametime;
        return trace;
    }
}
#endif
