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

#include "sv_phys_priv.h"

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
    BBox_t entBB = {
        .mins = ent->v.mins,
        .maxs = ent->v.maxs
    };
    if (ent->v.movetype == MOVETYPE_FLYMISSILE)     trace = SV_Move(ent->v.origin, entBB, end, MOVE_MISSILE, ent);
    else
        switch ((solid_t)ent->v.solid) {
        case SOLID_TRIGGER:  // only clip against bmodels
        case SOLID_NOT:                             trace = SV_Move(ent->v.origin, entBB, end, MOVE_NOMONSTERS, ent);    break;
        default:                                    trace = SV_Move(ent->v.origin, entBB, end, MOVE_NORMAL, ent);        break;
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
    edict_p moved_edict[EdictMax];
    vec3_t  moved_from[EdictMax];

    for (EdIdx e = EdictPlayer1; e < GetEdNum(); e++) {
        edict_p check = ED_GetEDictByIdx(e);
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

    vec3_t a = VectorSubtract(v3Zero, amove);
    vec3_t forward, right, up;  AngleVectors(a, forward, right, up);

    vec3_t pushorig = pusher->v.angles;

    // move the pusher to it's final position
    pusher->v.angles = VectorAdd(pusher->v.angles, amove);
    pusher->v.ltime += movetime;
    SV_LinkEdict(pusher, false);


    // see if any solid entities are inside the final position
    int num_moved = 0;
    edict_p moved_edict[EdictMax];
    vec3_t  moved_from[EdictMax];

    for (EdIdx e = EdictPlayer1; e < GetEdNum(); e++) {
        edict_p check = ED_GetEDictByIdx(e);

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
    SimDt_t movetime;
    if (thinktime < ent->v.ltime + host_frametime) {
        movetime = thinktime - ent->v.ltime;
        if (movetime < 0.f)
            movetime = 0.f;
    }
    else
        movetime = host_frametime;

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
