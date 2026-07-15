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
#include "BBox_tools.h"

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

    trace_t trace;
    if (ent->v.movetype == MOVETYPE_FLYMISSILE)     trace = SV_MoveDBox(ent->v.origin, push, MOVE_MISSILE, ent);
    else
        switch ((solid_t)ent->v.solid) {
        case SOLID_TRIGGER:  // only clip against bmodels
        case SOLID_NOT:                             trace = SV_MoveDBox(ent->v.origin, push, MOVE_NOMONSTERS, ent);    break;
        default:                                    trace = SV_MoveDBox(ent->v.origin, push, MOVE_NORMAL, ent);        break;
        }
        ent->v.origin = trace.endpos;
    SV_LinkEdict(ent, true);

    if (trace.pEnt)
        SV_Impact(ent, trace.pEnt);

    return trace;
}


/*
============
SV_PushMove

============
*/
void SV_PushMove(edict_p pusher, SimDt_t movetime) {
    if (VectorCompare(pusher->v.velocity, v3Zero)) {
        pusher->v.ltime += movetime;
        return;
    }

    vec3_t displacement = VectorScale(pusher->v.velocity, movetime);
    BBox_t bbPusher = BBoxFromVec3(
        VectorAdd(pusher->v.absmin, displacement),
        VectorAdd(pusher->v.absmax, displacement)
    );

    vec3_t pushorig = pusher->v.origin;

    // displacement the pusher to it's final position
    pusher->v.origin = VectorAdd(pusher->v.origin, displacement);
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

        default:    break;
        }

        // if the entity is standing on the pusher, it will definately be moved
        if (!(((EntityFlags_t)check->v.flags & FL_ONGROUND) &&
            ED_GetEDictByOffs(check->v.groundentity) == pusher)) {
#if 0
            if (
                (
                    (check->v.absmin.x >= maxs.x) ||
                    (check->v.absmin.y >= maxs.y) ||
                    (check->v.absmin.z >= maxs.z)) ||
                (
                    (check->v.absmax.x <= mins.x) ||
                    (check->v.absmax.y <= mins.y) ||
                    (check->v.absmax.z <= mins.z))
                )   continue;
#else
            if (!BBoxTouches(
                EvAbsBBox(&check->v),
                bbPusher)
                )   continue;
#endif
            // see if the ent's bbox is inside the pusher's final position
            if (!SV_TestEntityPosition(check))
                continue;
        }

        // remove the onground flag for non-players
        if (check->v.movetype != MOVETYPE_WALK)
            check->v.flags = (float)((EntityFlags_t)check->v.flags & ~FL_ONGROUND);

        vec3_t entorig = check->v.origin;
        moved_from[num_moved] = check->v.origin;
        moved_edict[num_moved] = check;
        num_moved++;

        // try moving the contacted entity
        pusher->v.solid = SOLID_NOT; {
            SV_PushEntity(check, displacement);
        } pusher->v.solid = SOLID_BSP;

        // if it is still inside the pusher, block
        edict_p block = SV_TestEntityPosition(check);
        if (block) { // fail the displacement
            if (check->v.mins.x == check->v.maxs.x)
                continue;

            if ((check->v.solid == SOLID_NOT) ||
                (check->v.solid == SOLID_TRIGGER)
                ) { // corpse
                check->v.mins.x = 0.f;
                check->v.mins.y = 0.f;
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
                pGame()->self = ED_GetEDictOffs(pusher);
                pGame()->other = ED_GetEDictOffs(check);
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
    if (AngleCompare(pusher->v.avelocity, a3Zero)) {
        pusher->v.ltime += movetime;
        return;
    }

    ang3_t amove = VectorScale(pusher->v.avelocity, movetime);

    ang3_t a = VectorSubtract(v3Zero, amove);
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

        switch ((movetype_t)check->v.movetype) {
        case MOVETYPE_PUSH:
        case MOVETYPE_NONE:
        case MOVETYPE_NOCLIP:
        case MOVETYPE_FOLLOW:   continue;
        default:                break;
        }

        // if the entity is standing on the pusher, it will definately be moved
        if (!(((EntityFlags_t)check->v.flags & FL_ONGROUND) &&
            ED_GetEDictByOffs(check->v.groundentity) == pusher)) {

            if (!BBoxTouches(
                EvAbsBBox(&check->v),
                EvAbsBBox(&pusher->v))
                )   continue;

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
        vec3_t displacement = VectorSubtract(org2, org);

        // try moving the contacted entity
        pusher->v.solid = SOLID_NOT;
        SV_PushEntity(check, displacement);
        pusher->v.solid = SOLID_BSP;

        // if it is still inside the pusher, block
        edict_p block = SV_TestEntityPosition(check);
        if (block) { // fail the move
            if (check->v.mins.x == check->v.maxs.x)   continue;

            if ((check->v.solid == SOLID_NOT) ||
                (check->v.solid == SOLID_TRIGGER)
                ) { // corpse
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
                pGame()->self = ED_GetEDictOffs(pusher);
                pGame()->other = ED_GetEDictOffs(check);
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
    if (thinktime < (ent->v.ltime + host_frametime)) {
        movetime = thinktime - ent->v.ltime;
        ClampLessThen(&movetime, 0.f);
    }
    else
        movetime = host_frametime;

    if (movetime) {
#ifdef QUAKE2
        if (ent->v.avelocity[PITCH] ||
            ent->v.avelocity[YAW] ||
            ent->v.avelocity[ROLL]
            )   SV_PushRotate(ent, movetime);
        else
#endif
            SV_PushMove(ent, movetime); // advances ent->v.ltime if not blocked
    }

    if ((thinktime > oldltime) &&
        (thinktime <= ent->v.ltime)
        ) {
        ent->v.nextthink = 0.f;
        pGame()->time = (float)SV_GetTime();
        pGame()->self = ED_GetEDictOffs(ent);
        pGame()->other = EdictWorld; // should be 0
        PR_ExecuteProgram(ent->v.think);
        if (ent->free)
            return;
    }

}
