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
    MOVECLIP_NONE = 0u,        /* no block */
    MOVECLIP_FLOOR = 1u << 0,   /* floor (normal[Z_AX] > 0) */
    MOVECLIP_WALL = 1u << 1,   /* wall/step (normal[Z_AX] == 0) */
    MOVECLIP_DEADSTOP = 1u << 2,   /* dead stop (reserved by original comment) */

    /* --- special early-return results (non-bitmask) --- */
    /* note: 3 == (MOVECLIP_FLOOR|MOVECLIP_WALL) by value; here it is used as “trapped/allsolid” */
    FLYMOVE_TRAPPED = MOVECLIP_FLOOR | MOVECLIP_WALL,   /* allsolid / clipped to too many planes */
    FLYMOVE_STUCK = FLYMOVE_TRAPPED | MOVECLIP_DEADSTOP /* unresolvable geometry / still stuck */
} MoveClipFlags_e;


#ifdef QUAKE2
static vec3_t _vecOrigin = { 0.0, 0.0, 0.0 };
#endif

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
        if (IS_NAN(ent->v.velocity[i])) {
            Con_Printf("Got a NaN velocity on %s\n", PR_GetQString(ent->v.classname));
            ent->v.velocity[i] = 0;
        }
        if (IS_NAN(ent->v.origin[i])) {
            Con_Printf("Got a NaN origin on %s\n", PR_GetQString(ent->v.classname));
            ent->v.origin[i] = 0;
        }
        CLAMP(-sv_maxvelocity.value, ent->v.velocity[i], sv_maxvelocity.value);
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
    pr_global_struct->other = ED_GetEDictOffs(Edicts);
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

MoveClipFlags_e ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce) {
    MoveClipFlags_e blocked = MOVECLIP_NONE;
    if (normal[Z_AX] > 0)  blocked |= MOVECLIP_FLOOR;  // floor
    if (!normal[Z_AX])     blocked |= MOVECLIP_WALL;  // step

    float backoff = DotProduct(in, normal) * overbounce;

    for (int i = 0; i < VECT_DIM; i++) {
        float change = normal[i] * backoff;
        out[i] = in[i] - change;
        if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
            out[i] = 0;
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
    vec3_t original_velocity;   VectorCopy(ent->v.velocity, original_velocity);
    vec3_t primal_velocity;     VectorCopy(ent->v.velocity, primal_velocity);
    int numplanes = 0;

    float time_left = time;

    int numbumps = 4;
    vec3_t planes[MAX_CLIP_PLANES];
    for (int bumpcount = 0; bumpcount < numbumps; bumpcount++) {
        if (!ent->v.velocity[X_AX] &&
            !ent->v.velocity[Y_AX] &&
            !ent->v.velocity[Z_AX]) // ent->v.velocity.is_zero()
            break;

        vec3_t end; // end + (timeleft * ent->v.velocity); in vect3df
        for (int i = 0; i < VECT_DIM; i++)
            end[i] = ent->v.origin[i] + time_left * ent->v.velocity[i];

        trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);

        if (trace.allsolid) { // entity is trapped in another solid
            VectorCopy(vec3_origin, ent->v.velocity);// ent->v.velocity = vec3_origin;
            return FLYMOVE_TRAPPED;
        }

        if (trace.fraction > 0) { // actually covered some distance
            VectorCopy(trace.endpos, ent->v.origin);
            VectorCopy(ent->v.velocity, original_velocity);
            numplanes = 0;
        }

        if (trace.fraction == 1)    break;  // moved the entire distance
        if (!trace.ent)             Host_SysError("SV_FlyMove: !trace.ent");

        if (trace.plane.normal[Z_AX] > 0.7) {
            blocked |= MOVECLIP_FLOOR;  // floor
            if (trace.ent->v.solid == SOLID_BSP) {
                ent->v.flags = (float)((EntityFlags_t)ent->v.flags | FL_ONGROUND);
                ent->v.groundentity = ED_GetEDictOffs(trace.ent);
            }
        }
        if (!trace.plane.normal[Z_AX]) {
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
            VectorCopy(vec3_origin, ent->v.velocity);
            return FLYMOVE_TRAPPED;
        }

        VectorCopy(trace.plane.normal, planes[numplanes]);
        numplanes++;

        //
        // modify original_velocity so it parallels all of the clip planes
        //
        vec3_t  new_velocity;
        int i = 0;
        for (; i < numplanes; i++) {
            ClipVelocity(original_velocity, planes[i], new_velocity, 1);
            int j = 0;
            for (; j < numplanes; j++)
                if ((j != i) &&
                    (DotProduct(new_velocity, planes[j]) < 0))
                    break; // not ok

            if (j == numplanes)     break;
        }

        if (i != numplanes) { // go along this plane
            VectorCopy(new_velocity, ent->v.velocity);
        }
        else { // go along the crease
            if (numplanes != 2) {
                //    Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
                VectorCopy(vec3_origin, ent->v.velocity);
                return FLYMOVE_STUCK;
            }
            vec3_t dir; CrossProduct(planes[X_AX], planes[Y_AX], dir);
            float d = DotProduct(dir, ent->v.velocity);
            VectorScale(dir, d, ent->v.velocity);
        }

        //
        // if original velocity is against the original velocity, stop dead
        // to avoid tiny occilations in sloping corners
        //
        if (DotProduct(ent->v.velocity, primal_velocity) <= 0) {
            VectorCopy(vec3_origin, ent->v.velocity);
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
    ent->v.velocity[Z_AX] -= (float)(ent_gravity * sv_gravity.value * host_frametime);
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
    vec3_t end; VectorAdd(ent->v.origin, push, end);

    trace_t trace;
    if (ent->v.movetype == MOVETYPE_FLYMISSILE)     trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_MISSILE, ent);
    else
        switch ((solid_t)ent->v.solid) {
        case SOLID_TRIGGER:  // only clip against bmodels
        case SOLID_NOT:                             trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NOMONSTERS, ent);    break;
        default:                                    trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);        break;
        }
    VectorCopy(trace.endpos, ent->v.origin);
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
    if (!pusher->v.velocity[X_AX] &&
        !pusher->v.velocity[Y_AX] &&
        !pusher->v.velocity[Z_AX]) {
        pusher->v.ltime += movetime;
        return;
    }

    vec3_t  mins, maxs, move;
    for (int i = 0; i < VECT_DIM; i++) {
        move[i] = pusher->v.velocity[i] * movetime;
        mins[i] = pusher->v.absmin[i] + move[i];
        maxs[i] = pusher->v.absmax[i] + move[i];
    }

    vec3_t pushorig;    VectorCopy(pusher->v.origin, pushorig);

    // move the pusher to it's final position
    VectorAdd(pusher->v.origin, move, pusher->v.origin);
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
                    check->v.absmin[X_AX] >= maxs[X_AX] ||
                    check->v.absmin[Y_AX] >= maxs[Y_AX] ||
                    check->v.absmin[Z_AX] >= maxs[Z_AX]) ||
                (
                    check->v.absmax[X_AX] <= mins[X_AX] ||
                    check->v.absmax[Y_AX] <= mins[Y_AX] ||
                    check->v.absmax[Z_AX] <= mins[Z_AX])
                )
                continue;

            // see if the ent's bbox is inside the pusher's final position
            if (!SV_TestEntityPosition(check))
                continue;
        }

        // remove the onground flag for non-players
        if (check->v.movetype != MOVETYPE_WALK)
            check->v.flags = (float)((int)((EntityFlags_t)check->v.flags) & ~FL_ONGROUND);

        vec3_t entorig; VectorCopy(check->v.origin, entorig);
        VectorCopy(check->v.origin, moved_from[num_moved]);
        moved_edict[num_moved] = check;
        num_moved++;

        // try moving the contacted entity
        pusher->v.solid = SOLID_NOT;
        SV_PushEntity(check, move);
        pusher->v.solid = SOLID_BSP;

        // if it is still inside the pusher, block
        edict_p block = SV_TestEntityPosition(check);
        if (block) { // fail the move
            if (check->v.mins[X_AX] == check->v.maxs[X_AX]) continue;

            if (check->v.solid == SOLID_NOT ||
                check->v.solid == SOLID_TRIGGER) { // corpse
                check->v.mins[X_AX] = check->v.mins[Y_AX] = 0;
                VectorCopy(check->v.mins, check->v.maxs);
                continue;
            }

            VectorCopy(entorig, check->v.origin);
            SV_LinkEdict(check, true);

            VectorCopy(pushorig, pusher->v.origin);
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
                VectorCopy(moved_from[i], moved_edict[i]->v.origin);
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
    if (!pusher->v.avelocity[X_AX] &&
        !pusher->v.avelocity[Y_AX] &&
        !pusher->v.avelocity[Z_AX]) {
        pusher->v.ltime += movetime;
        return;
    }

    vec3_t amove;
    for (int i = 0; i < VECT_DIM; i++)
        amove[i] = pusher->v.avelocity[i] * movetime;

    vec3_t a;   VectorSubtract(vec3_origin, amove, a);
    vec3_t forward, right, up;  AngleVectors(a, forward, right, up);

    vec3_t pushorig;    VectorCopy(pusher->v.angles, pushorig);

    // move the pusher to it's final position
    VectorAdd(pusher->v.angles, amove, pusher->v.angles);
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
                    check->v.absmin[X_AX] >= pusher->v.absmax[X_AX] ||
                    check->v.absmin[Y_AX] >= pusher->v.absmax[Y_AX] ||
                    check->v.absmin[Z_AX] >= pusher->v.absmax[Z_AX]) ||
                (
                    check->v.absmax[X_AX] <= pusher->v.absmin[X_AX] ||
                    check->v.absmax[Y_AX] <= pusher->v.absmin[Y_AX] ||
                    check->v.absmax[Z_AX] <= pusher->v.absmin[Z_AX])
                )
                continue;

            // see if the ent's bbox is inside the pusher's final position
            if (!SV_TestEntityPosition(check))
                continue;
        }

        // remove the onground flag for non-players
        if (check->v.movetype != MOVETYPE_WALK)
            check->v.flags = (EntityFlags_t)check->v.flags & ~FL_ONGROUND;

        vec3_t  entorig;    VectorCopy(check->v.origin, entorig);
        VectorCopy(check->v.origin, moved_from[num_moved]);
        moved_edict[num_moved] = check;
        num_moved++;

        // calculate destination position
        vec3_t org; VectorSubtract(check->v.origin, pusher->v.origin, org);
        vec3_t org2 = {
            DotProduct(org, forward),
            -DotProduct(org, right),
            DotProduct(org, up)
        };
        vec3_t  move;   VectorSubtract(org2, org, move);

        // try moving the contacted entity
        pusher->v.solid = SOLID_NOT;
        SV_PushEntity(check, move);
        pusher->v.solid = SOLID_BSP;

        // if it is still inside the pusher, block
        edict_p block = SV_TestEntityPosition(check);
        if (block) { // fail the move
            if (check->v.mins[X_AX] == check->v.maxs[X_AX])   continue;

            if ((check->v.solid == SOLID_NOT) ||
                (check->v.solid == SOLID_TRIGGER)) { // corpse
                check->v.mins[X_AX] = check->v.mins[Y_AX] = 0;
                VectorCopy(check->v.mins, check->v.maxs);
                continue;
            }

            VectorCopy(entorig, check->v.origin);
            SV_LinkEdict(check, true);

            VectorCopy(pushorig, pusher->v.angles);
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
                VectorCopy(moved_from[i], moved_edict[i]->v.origin);
                VectorSubtract(moved_edict[i]->v.angles, amove, moved_edict[i]->v.angles);
                SV_LinkEdict(moved_edict[i], false);
            }
            return;
        }
        else {
            VectorAdd(check->v.angles, amove, check->v.angles);
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
        pr_global_struct->other = ED_GetEDictOffs(Edicts);
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
        VectorCopy(ent->v.origin, ent->v.oldorigin);
        return;
    }

    vec3_t org; VectorCopy(ent->v.origin, org);
    VectorCopy(ent->v.oldorigin, ent->v.origin);
    if (!SV_TestEntityPosition(ent)) {
        Con_DPrintf("Unstuck.\n");
        SV_LinkEdict(ent, true);
        return;
    }

    for (int z = 0; z < 18; z++)
        for (int i = -1; i <= 1; i++)
            for (int j = -1; j <= 1; j++) {
                ent->v.origin[X_AX] = org[X_AX] + (float)i;
                ent->v.origin[Y_AX] = org[Y_AX] + (float)j;
                ent->v.velocity[Z_AX] = org[Z_AX] + (float)z;
                if (!SV_TestEntityPosition(ent)) {
                    Con_DPrintf("Unstuck.\n");
                    SV_LinkEdict(ent, true);
                    return;
                }
            }

    VectorCopy(org, ent->v.origin);
    Con_DPrintf("player is stuck.\n");
}


/*
=============
SV_CheckWater
=============
*/
bool SV_CheckWater(edict_p ent) {
    vec3_t point = {
        ent->v.origin[X_AX],
        ent->v.origin[Y_AX],
        ent->v.velocity[Z_AX] + ent->v.mins[Z_AX] + 1
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
        point[Z_AX] = ent->v.origin[Z_AX] + (ent->v.mins[Z_AX] + ent->v.maxs[Z_AX]) * 0.5f;
        cont = SV_PointContents(point);
        if (cont <= CONTENTS_WATER) {
            ent->v.waterlevel = 2;
            point[Z_AX] = ent->v.origin[Z_AX] + ent->v.view_ofs[Z_AX];
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

            VectorMA(ent->v.basevelocity, 150.0 * ent->v.waterlevel / 3.0, current_table[CONTENTS_CURRENT_0 - truecont], ent->v.basevelocity);
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
    vec3_t  forward, right, up; AngleVectors(ent->v.v_angle, forward, right, up);
    float d = DotProduct(trace->plane.normal, forward);

    d += 0.5f;
    if (d >= 0)
        return;

    // cut the tangential velocity
    float i = DotProduct(trace->plane.normal, ent->v.velocity);
    vec3_t into;    VectorScale(trace->plane.normal, i, into);
    vec3_t side;    VectorSubtract(ent->v.velocity, into, side);

    ent->v.velocity[X_AX] = side[X_AX] * (1 + d);
    ent->v.velocity[Y_AX] = side[Y_AX] * (1 + d);
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
    vec3_t oldorg;  VectorCopy(ent->v.origin, oldorg);
    vec3_t dir;     VectorCopy(vec3_origin, dir);

    for (int i = 0; i < 8; i++) {
        // try pushing a little in an axial direction
        switch (i) {
        case 0:     dir[X_AX] = 2;  dir[Y_AX] = 0;  break;
        case 1:     dir[X_AX] = 0;  dir[Y_AX] = 2;  break;
        case 2:     dir[X_AX] = -2; dir[Y_AX] = 0;  break;
        case 3:     dir[X_AX] = 0;  dir[Y_AX] = -2; break;
        case 4:     dir[X_AX] = 2;  dir[Y_AX] = 2;  break;
        case 5:     dir[X_AX] = -2; dir[Y_AX] = 2;  break;
        case 6:     dir[X_AX] = 2;  dir[Y_AX] = -2; break;
        case 7:     dir[X_AX] = -2; dir[Y_AX] = -2; break;
        }

        SV_PushEntity(ent, dir);

        // retry the original move
        ent->v.velocity[X_AX] = oldvel[X_AX];
        ent->v.velocity[Y_AX] = oldvel[Y_AX];
        ent->v.velocity[Z_AX] = 0;
        trace_t steptrace;
        MoveClipFlags_e clip = SV_FlyMove(ent, 0.1f, &steptrace);

        if ((fabs(oldorg[Y_AX] - ent->v.origin[Y_AX]) > 4) ||
            (fabs(oldorg[X_AX] - ent->v.origin[X_AX]) > 4)) {
            //Con_DPrintf ("unstuck!\n");
            return clip;
        }

        // go back to the original pos and try again
        VectorCopy(oldorg, ent->v.origin);
    }

    VectorCopy(vec3_origin, ent->v.velocity);
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

    vec3_t oldorg;  VectorCopy(ent->v.origin, oldorg);
    vec3_t oldvel;  VectorCopy(ent->v.velocity, oldvel);

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

    vec3_t nosteporg;   VectorCopy(ent->v.origin, nosteporg);
    vec3_t nostepvel;   VectorCopy(ent->v.velocity, nostepvel);

    // try moving up and forward to go up a step
    VectorCopy(oldorg, ent->v.origin); // back to start pos

    vec3_t upmove;      VectorCopy(vec3_origin, upmove);
    vec3_t downmove;    VectorCopy(vec3_origin, downmove);
    upmove[Z_AX] = STEPSIZE;
    downmove[Z_AX] = (float)(-STEPSIZE + oldvel[Z_AX] * host_frametime);

    // move up
    SV_PushEntity(ent, upmove); // FIXME: don't link?

    // move forward
    ent->v.velocity[X_AX] = oldvel[X_AX];
    ent->v.velocity[Y_AX] = oldvel[Y_AX];
    ent->v.velocity[Z_AX] = 0;
    MoveClipFlags_e clip = SV_FlyMove(ent, (float)host_frametime, &steptrace);

    // check for stuckness, possibly due to the limited precision of floats
    // in the clipping hulls
    if (clip) {
        if ((fabs(oldorg[Y_AX] - ent->v.origin[Y_AX]) < 0.03125) &&
            (fabs(oldorg[X_AX] - ent->v.origin[X_AX]) < 0.03125)) { // stepping up didn't make any progress
            clip = SV_TryUnstick(ent, oldvel);
        }
    }

    // extra friction based on view angle
    if (clip & MOVECLIP_WALL)   SV_WallFriction(ent, &steptrace);

    // move down
    trace_t downtrace = SV_PushEntity(ent, downmove); // FIXME: don't link?

    if (downtrace.plane.normal[Z_AX] > 0.7) {
        if (ent->v.solid == SOLID_BSP) {
            ent->v.flags = (float)((int)((EntityFlags_t)ent->v.flags) | FL_ONGROUND);
            ent->v.groundentity = ED_GetEDictOffs(downtrace.ent);
        }
    }
    else {
        // if the push down didn't end up on good ground, use the move without
        // the step up.  This happens near wall / slope combinations, and can
        // cause the player to hop up higher on a slope too steep to climb
        VectorCopy(nosteporg, ent->v.origin);
        VectorCopy(nostepvel, ent->v.velocity);
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
            !((EntityFlags_t)ent->v.flags & FL_WATERJUMP))
            SV_AddGravity(ent);

        SV_CheckStuck(ent);
#ifdef QUAKE2
        VectorAdd(ent->v.velocity, ent->v.basevelocity, ent->v.velocity); // ent->v.velocity += ent->v.basevelocity;
        SV_WalkMove(ent);
        VectorSubtract(ent->v.velocity, ent->v.basevelocity, ent->v.velocity); // ent->v.velocity -= ent->v.basevelocity;
#else
        SV_WalkMove(ent);
#endif
        break;

    case MOVETYPE_TOSS:
    case MOVETYPE_BOUNCE:   SV_Physics_Toss(ent);                   break;
    case MOVETYPE_FLY:      SV_FlyMove(ent, (float)host_frametime, NULL);  break;
    case MOVETYPE_NOCLIP:   VectorMA(ent->v.origin, (float)host_frametime, ent->v.velocity, ent->v.origin);    break;
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
    VectorAdd(ED_GetEDictByOffs(ent->v.aiment)->v.origin, ent->v.v_angle, ent->v.origin);
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

    VectorMA(ent->v.angles, (float)host_frametime, ent->v.avelocity, ent->v.angles);
    VectorMA(ent->v.origin, (float)host_frametime, ent->v.velocity, ent->v.origin);

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
void SV_Physics_Toss(edict_p ent) {
#ifdef QUAKE2
    edict_p groundentity = ED_GetEDictByOffs(ent->v.groundentity);
    if ((EntityFlags_t)groundentity->v.flags & FL_CONVEYOR) VectorScale(groundentity->v.movedir, groundentity->v.speed, ent->v.basevelocity);
    else                                                    VectorCopy(_vecOrigin, ent->v.basevelocity);
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
    VectorMA(ent->v.angles, (float)host_frametime, ent->v.avelocity, ent->v.angles);

    // move origin
#ifdef QUAKE2
    VectorAdd(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);
#endif
    vec3_t move;    VectorScale(ent->v.velocity, (float)host_frametime, move);
    trace_t trace = SV_PushEntity(ent, move);
#ifdef QUAKE2
    VectorSubtract(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);
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

    ClipVelocity(ent->v.velocity, trace.plane.normal, ent->v.velocity, backoff);

    // stop if on ground
    if ((trace.plane.normal[Z_AX] > 0.7) &&
        ((ent->v.velocity[Z_AX] < 60) ||
            (
                (ent->v.movetype != MOVETYPE_BOUNCE)
#ifdef QUAKE2
                && (ent->v.movetype != MOVETYPE_BOUNCEMISSILE)
#endif
                )
            )) {
        ent->v.flags = (float)((int)((EntityFlags_t)ent->v.flags) | FL_ONGROUND);
        ent->v.groundentity = ED_GetEDictOffs(trace.ent);
        VectorCopy(vec3_origin, ent->v.velocity);
        VectorCopy(vec3_origin, ent->v.avelocity);

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
    if ((EntityFlags_t)groundentity->v.flags & FL_CONVEYOR)   VectorScale(groundentity->v.movedir, groundentity->v.speed, ent->v.basevelocity);
    else                                            VectorCopy(_vecOrigin, ent->v.basevelocity);
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
                float_p vel = ent->v.velocity;
                float speed = sqrt(vel[X_AX] * vel[X_AX] + vel[Y_AX] * vel[Y_AX]);
                if (speed) {
                    float friction = sv_friction.value;

                    float control = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
                    float newspeed = speed - host_frametime * control * friction;

                    if (newspeed < 0)
                        newspeed = 0;
                    newspeed /= speed;

                    vel[X_AX] = vel[X_AX] * newspeed;
                    vel[Y_AX] = vel[Y_AX] * newspeed;
                }
            }

        VectorAdd(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);
        SV_FlyMove(ent, host_frametime, NULL);
        VectorSubtract(ent->v.velocity, ent->v.basevelocity, ent->v.velocity);

        // determine if it's on solid ground at all
        {
            vec3_t mins; VectorAdd(ent->v.origin, ent->v.mins, mins);
            vec3_t maxs; VectorAdd(ent->v.origin, ent->v.maxs, maxs);

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
        bool hitsound = (ent->v.velocity[Z_AX] < sv_gravity.value * -0.1);

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
    pr_global_struct->self = ED_GetEDictOffs(Edicts);
    pr_global_struct->other = ED_GetEDictOffs(Edicts);
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
    LegacyTimeStamp_t save_frametime = host_frametime;
    host_frametime = 0.05;

    edict_t tempent; memcpy(&tempent, ent, sizeof(edict_t));
    edict_p tent = &tempent;

    while (1) {
        SV_CheckVelocity(tent);
        SV_AddGravity(tent);
        VectorMA(tent->v.angles, (float)host_frametime, tent->v.avelocity, tent->v.angles);
        vec3_t move;    VectorScale(tent->v.velocity, (float)host_frametime, move);
        vec3_t end;     VectorAdd(tent->v.origin, move, end);
        trace_t trace = SV_Move(tent->v.origin, tent->v.mins, tent->v.maxs, end, MOVE_NORMAL, tent);
        VectorCopy(trace.endpos, tent->v.origin);

#if 0
        exter_n Particle_p active_particles, free_particles;
        Particle_p p = free_particles;
        if (p) {
            free_particles = p->next;
            p->next = active_particles;
            active_particles = p;

            p->die = 256;
            p->color = 15;
            p->type = pt_static;
            VectorCopy(vec3_origin, p->vel);
            VectorCopy(tent->v.origin, p->org);
        }
#endif

        if ((trace.ent) &&
            (trace.ent != ignore))
            // p->color = 224;
            host_frametime = save_frametime;
        return trace;
    }
}
#endif
