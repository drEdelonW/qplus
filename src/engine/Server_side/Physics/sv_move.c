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
// sv_move.c -- monster movement

#include "server.h"
#include "world.h"
#include "mathlib.h"
#include <stdlib.h>
#include "progs.h"
#include "GlobVars.h"

#define STEPSIZE (18)

/*
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
// static int c_yes, c_no;

bool SV_CheckBottom(edict_p ent) {
    vec3_t mins = VectorAdd(ent->v.origin, ent->v.mins);
    vec3_t maxs = VectorAdd(ent->v.origin, ent->v.maxs);


    // if all of the points under the corners are solid world, don't bother with the tougher checks
    // the corners must be within 16 of the midpoint
    vec3_t start = {
        .x = 0.0f,
        .y = 0.0f,
        .z = mins.z - 1.0f
    };
    for (int x = 0; x <= 1; x++)
        for (int y = 0; y <= 1; y++) {
            start.x = (x) ? maxs.x : mins.x;
            start.y = (y) ? maxs.y : mins.y;
            if (SV_PointContents(start) != CONTENTS_SOLID)
                goto realcheck;
        }

    // c_yes++;
    return true;  // we got out easy

realcheck:
    // c_no++;
    //
    // check it for real...
    //
#if 0
    start.z = mins.z;

    // the midpoint must be within 16 of the bottom
    vec3_t stop = {
        .x = start.x = (mins.x + maxs.x) * 0.5f,
        .y = start.y = (mins.y + maxs.y) * 0.5f,
        .z = start.z - 2 * STEPSIZE
    };
#else
    start = (vec3_t){
        .x = (mins.x + maxs.x) * 0.5f,
        .y = (mins.y + maxs.y) * 0.5f,
        .z = mins.z   // the midpoint must be within 16 of the bottom
    };

    vec3_t stop = {
        .x = start.x,
        .y = start.y,
        .z = start.z - 2 * STEPSIZE
    };
#endif
    trace_t trace = SV_Move(start, vec3_origin, vec3_origin, stop, MOVE_NOMONSTERS, ent);

    if (trace.fraction == 1.0)
        return false;

    float mid = trace.endpos.z;
    float bottom = trace.endpos.z;

    // the corners must be within 16 of the midpoint
    for (int x = 0; x <= 1; x++)
        for (int y = 0; y <= 1; y++) {
            start.x = stop.x = (x) ? maxs.x : mins.x;
            start.y = stop.y = (y) ? maxs.y : mins.y;

            trace = SV_Move(start, vec3_origin, vec3_origin, stop, MOVE_NOMONSTERS, ent);

            if ((trace.fraction != 1.0) &&
                (trace.endpos.z > bottom)
                )
                bottom = trace.endpos.z;
            if ((trace.fraction == 1.0) ||
                (mid - trace.endpos.z > STEPSIZE)
                )
                return false;
        }

    // c_yes++;
    return true;
}


/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
bool SV_movestep(edict_p ent, vec3_t move, bool relink) {
    // try the move
    vec3_t oldorg = ent->v.origin;
    vec3_t neworg = VectorAdd(ent->v.origin, move);


    // flying monsters don't step up
    if ((int)ent->v.flags & (FL_SWIM | FL_FLY)) {
        // try one move with vertical motion, then one without
        for (int i = 0; i < 2; i++) {
            neworg = VectorAdd(ent->v.origin, move);
            edict_p enemy = ED_GetEDictByOffs(ent->v.enemy);
            if (i == 0 && (enemy != Edicts)) {
                float dz = ent->v.origin.z - ED_GetEDictByOffs(ent->v.enemy)->v.origin.z;
                if (dz > 40)    neworg.z -= 8;
                if (dz < 30)    neworg.z += 8;
            }
            trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, neworg, MOVE_NORMAL, ent);

            if (trace.fraction == 1) {
                if (((int)ent->v.flags & FL_SWIM) &&
                    SV_PointContents(trace.endpos) == CONTENTS_EMPTY)
                    return false; // swim monster left water

                ent->v.origin = trace.endpos;
                if (relink)
                    SV_LinkEdict(ent, true);
                return true;
            }

            if (enemy == Edicts)
                break;
        }

        return false;
    }

    // push down from a step height above the wished position
    neworg.z += STEPSIZE;
    vec3_t end = neworg;
    end.z -= STEPSIZE * 2.0f;

    trace_t trace = SV_Move(neworg, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);

    if (trace.allsolid)
        return false;

    if (trace.startsolid) {
        neworg.z -= STEPSIZE;
        trace = SV_Move(neworg, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);
        if (trace.allsolid || trace.startsolid)
            return false;
    }
    if (trace.fraction == 1) {
        // if monster had the ground pulled out, go ahead and fall
        if ((int)ent->v.flags & FL_PARTIALGROUND) {
            ent->v.origin = VectorAdd(ent->v.origin, move);
            if (relink)
                SV_LinkEdict(ent, true);
            ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;
            // Con_Printf ("fall down\n");
            return true;
        }

        return false;  // walked off an edge
    }

    // check point traces down for dangling corners
    ent->v.origin = trace.endpos;

    if (!SV_CheckBottom(ent)) {
        if ((int)ent->v.flags & FL_PARTIALGROUND) { // entity had floor mostly pulled out from underneath it
            // and is trying to correct
            if (relink)
                SV_LinkEdict(ent, true);
            return true;
        }
        ent->v.origin = oldorg;
        return false;
    }

    if ((int)ent->v.flags & FL_PARTIALGROUND) {
        //  Con_Printf ("back on ground\n");
        ent->v.flags = (int)ent->v.flags & ~FL_PARTIALGROUND;
    }
    ent->v.groundentity = ED_GetEDictOffs(trace.ent);

    // the move is ok
    if (relink)
        SV_LinkEdict(ent, true);
    return true;
}


//============================================================================

/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
void PF_changeyaw();
bool SV_StepDirection(edict_p ent, float yaw, float dist) {
    ent->v.ideal_yaw = yaw;
    PF_changeyaw();

    yaw = yaw * (float)M_PI * 2 / 360;
    vec3_t move = {
        .x = (float)cos(yaw) * dist,
        .y = (float)sin(yaw) * dist,
        .z = 0.0f
    };

    vec3_t oldorigin = ent->v.origin;
    if (SV_movestep(ent, move, false)) {
        float delta = ent->v.angles.yaw - ent->v.ideal_yaw;
        if ((delta > 45) &&
            (delta < 315)  // not turned far enough, so don't take the step
            ) {
            ent->v.origin = oldorigin;
        }
        SV_LinkEdict(ent, true);
        return true;
    }
    SV_LinkEdict(ent, true);

    return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
void SV_FixCheckBottom(edict_p ent) {
    // Con_Printf ("SV_FixCheckBottom\n");

    ent->v.flags = (int)ent->v.flags | FL_PARTIALGROUND;
}



/*
================
SV_NewChaseDir

================
*/
#define DI_NODIR -1.0f
void SV_NewChaseDir(edict_p actor, edict_p enemy, float dist) {
    // float   orient[3];   // direction angle Euler
    vec3_t  orient;   // direction angle Euler

    float olddir = anglemod((float)((int)(actor->v.ideal_yaw / 45) * 45));
    float turnaround = anglemod(olddir - 180);

    float deltax = enemy->v.origin.pitch - actor->v.origin.pitch;
    float deltay = enemy->v.origin.yaw - actor->v.origin.yaw;

#if 0
    if (deltax > 10.0f)         orient.yaw = 0.0f;
    else if (deltax < -10.0f)   orient.yaw = 180.0f;
    else                        orient.yaw = DI_NODIR;

    if (deltay < -10.0f)        orient.roll = 270.0f;
    else if (deltay > 10.0f)    orient.roll = 90.0f;
    else                        orient.roll = DI_NODIR;
#else
    orient.yaw =
        (deltax > 10.0f) ?
        0.0f : ((deltax < -10.0f) ?
            180.0f : DI_NODIR);

    orient.roll =
        (deltay < -10.0f) ?
        270.0f : ((deltay > 10.0f) ?
            90.0f : DI_NODIR);
#endif
    // try direct route
    if ((orient.yaw != DI_NODIR) &&
        (orient.roll != DI_NODIR)
        ) {
        float tdir;
        if (orient.yaw == 0.0f)  tdir = (orient.roll == 90.0f) ? 45.0f : 315.0f;
        else                        tdir = (orient.roll == 90.0f) ? 135.0f : 215.0f;

        if ((tdir != turnaround) &&
            SV_StepDirection(actor, tdir, dist)
            )
            return;
    }

    // try other directions
    if (((rand() & 3) & 1) ||
        (fabs(deltay) > fabs(deltax))
        ) {
        float tdir = orient.yaw;
        orient.yaw = orient.roll;
        orient.roll = tdir;
    }

    if (
        (
            (orient.yaw != DI_NODIR) &&
            (orient.yaw != turnaround) &&
            SV_StepDirection(actor, orient.yaw, dist)
            ) ||
        (
            (orient.roll != DI_NODIR) &&
            (orient.roll != turnaround) &&
            SV_StepDirection(actor, orient.roll, dist)
            )
        )
        return;

    /* there is no direct path to the player, so pick another direction */

    if ((olddir != DI_NODIR) &&
        SV_StepDirection(actor, olddir, dist)
        )
        return;

    {
        if (rand() & 1) {  /*randomly determine direction of search*/
            for (float tdir = 0.0f; tdir <= 315.0f; tdir += 45.0f)
                if ((tdir != turnaround) &&
                    SV_StepDirection(actor, tdir, dist)
                    )
                    return;
        }
        else {
            for (float tdir = 315.0f; tdir >= 0.0f; tdir -= 45.0f)
                if ((tdir != turnaround) &&
                    SV_StepDirection(actor, tdir, dist)
                    )
                    return;
        }
    }

    if ((turnaround != DI_NODIR) &&
        SV_StepDirection(actor, turnaround, dist)
        )
        return;

    actor->v.ideal_yaw = olddir;  // can't move

    // if a bridge was pulled out from underneath a monster, it may not have
    // a valid standing position at all

    if (!SV_CheckBottom(actor))
        SV_FixCheckBottom(actor);

}

/*
======================
SV_CloseEnough

======================
*/
bool SV_CloseEnough(edict_p ent, edict_p goal, float dist) {
    for (int i = 0; i < VECT_DIM; i++) {
        if ((goal->v.absmin.v[i] > (ent->v.absmax.v[i] + dist)) ||
            (goal->v.absmax.v[i] < (ent->v.absmin.v[i] - dist)))
            return false;
    }
    return true;
}

/*
======================
SV_MoveToGoal

======================
*/
void SV_MoveToGoal() {
    edict_p ent = ED_GetEDictByOffs(pr_global_struct->self);
    edict_p goal = ED_GetEDictByOffs(ent->v.goalentity);
    float dist = G_FLOAT(OFS_PARM0);

    if (!((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM))) {
        PR_Freturn 0;
        return;
    }

    // if the next step hits the enemy, return immediately
#ifdef QUAKE2
    edict_p enemy = ED_GetEDictByOffs(ent->v.enemy);
    if ((enemy != Edicts) &&
        SV_CloseEnough(ent, enemy, dist))
#else
    if ((ED_GetEDictByOffs(ent->v.enemy) != Edicts) &&
        SV_CloseEnough(ent, goal, dist))
#endif
        return;

    // bump around...
    if ((rand() & 3) == 1 ||
        !SV_StepDirection(ent, ent->v.ideal_yaw, dist)) {
        SV_NewChaseDir(ent, goal, dist);
    }
}

