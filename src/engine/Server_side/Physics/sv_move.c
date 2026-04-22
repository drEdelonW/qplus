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
    vec3_t mins;    VectorAdd(ent->v.origin, ent->v.mins, mins);
    vec3_t maxs;    VectorAdd(ent->v.origin, ent->v.maxs, maxs);


    // if all of the points under the corners are solid world, don't bother
    // with the tougher checks
    // the corners must be within 16 of the midpoint
    vec3_t start = { 0, 0, mins[Z_AX] - 1 };
    for (int x = 0; x <= 1; x++)
        for (int y = 0; y <= 1; y++) {
            start[X_AX] = x ? maxs[X_AX] : mins[X_AX];
            start[Y_AX] = y ? maxs[Y_AX] : mins[Y_AX];
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
    start[Z_AX] = mins[Z_AX];

    // the midpoint must be within 16 of the bottom
    vec3_t stop;
    start[X_AX] = stop[X_AX] = (mins[X_AX] + maxs[X_AX]) * 0.5f;
    start[Y_AX] = stop[Y_AX] = (mins[Y_AX] + maxs[Y_AX]) * 0.5f;
    stop[Z_AX] = start[Z_AX] - 2 * STEPSIZE;
    trace_t trace = SV_Move(start, vec3_origin, vec3_origin, stop, MOVE_NOMONSTERS, ent);

    if (trace.fraction == 1.0)
        return false;

    float mid = trace.endpos[Z_AX];
    float bottom = trace.endpos[Z_AX];

    // the corners must be within 16 of the midpoint
    for (int x = 0; x <= 1; x++)
        for (int y = 0; y <= 1; y++) {
            start[X_AX] = stop[X_AX] = x ? maxs[X_AX] : mins[X_AX];
            start[Y_AX] = stop[Y_AX] = y ? maxs[Y_AX] : mins[Y_AX];

            trace = SV_Move(start, vec3_origin, vec3_origin, stop, MOVE_NOMONSTERS, ent);

            if ((trace.fraction != 1.0) && (trace.endpos[Z_AX] > bottom))
                bottom = trace.endpos[Z_AX];
            if ((trace.fraction == 1.0) || (mid - trace.endpos[Z_AX] > STEPSIZE))
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
    vec3_t oldorg;  VectorCopy(ent->v.origin, oldorg);
    vec3_t neworg;  VectorAdd(ent->v.origin, move, neworg);


    // flying monsters don't step up
    if ((int)ent->v.flags & (FL_SWIM | FL_FLY)) {
        // try one move with vertical motion, then one without
        for (int i = 0; i < 2; i++) {
            VectorAdd(ent->v.origin, move, neworg);
            edict_p enemy = ED_GetEDictByOffs(ent->v.enemy);
            if (i == 0 && (enemy != Edicts)) {
                float dz = ent->v.origin[Z_AX] - ED_GetEDictByOffs(ent->v.enemy)->v.origin[Z_AX];
                if (dz > 40)    neworg[Z_AX] -= 8;
                if (dz < 30)    neworg[Z_AX] += 8;
            }
            trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, neworg, MOVE_NORMAL, ent);

            if (trace.fraction == 1) {
                if (((int)ent->v.flags & FL_SWIM) &&
                    SV_PointContents(trace.endpos) == CONTENTS_EMPTY)
                    return false; // swim monster left water

                VectorCopy(trace.endpos, ent->v.origin);
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
    neworg[Z_AX] += STEPSIZE;
    vec3_t end; VectorCopy(neworg, end);
    end[Z_AX] -= STEPSIZE * 2;

    trace_t trace = SV_Move(neworg, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);

    if (trace.allsolid)
        return false;

    if (trace.startsolid) {
        neworg[Z_AX] -= STEPSIZE;
        trace = SV_Move(neworg, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);
        if (trace.allsolid || trace.startsolid)
            return false;
    }
    if (trace.fraction == 1) {
        // if monster had the ground pulled out, go ahead and fall
        if ((int)ent->v.flags & FL_PARTIALGROUND) {
            VectorAdd(ent->v.origin, move, ent->v.origin);
            if (relink)
                SV_LinkEdict(ent, true);
            ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;
            // Con_Printf ("fall down\n");
            return true;
        }

        return false;  // walked off an edge
    }

    // check point traces down for dangling corners
    VectorCopy(trace.endpos, ent->v.origin);

    if (!SV_CheckBottom(ent)) {
        if ((int)ent->v.flags & FL_PARTIALGROUND) { // entity had floor mostly pulled out from underneath it
            // and is trying to correct
            if (relink)
                SV_LinkEdict(ent, true);
            return true;
        }
        VectorCopy(oldorg, ent->v.origin);
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
    vec3_t  move = {
        (float)cos(yaw) * dist,
        (float)sin(yaw) * dist,
        0
    };

    vec3_t oldorigin;   VectorCopy(ent->v.origin, oldorigin);
    if (SV_movestep(ent, move, false)) {
        float delta = ent->v.angles[YAW] - ent->v.ideal_yaw;
        if (delta > 45 && delta < 315) {  // not turned far enough, so don't take the step
            VectorCopy(oldorigin, ent->v.origin);
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
#define DI_NODIR -1
void SV_NewChaseDir(edict_p actor, edict_p enemy, float dist) {
    // float   orient[3];   // direction angle Euler
    vec3_t  orient;   // direction angle Euler

    float olddir = anglemod((float)((int)(actor->v.ideal_yaw / 45) * 45));
    float turnaround = anglemod(olddir - 180);

    float deltax = enemy->v.origin[X_AX] - actor->v.origin[X_AX];
    if (deltax > 10)        orient[Y_AX] = 0;
    else if (deltax < -10)  orient[Y_AX] = 180;
    else                    orient[Y_AX] = DI_NODIR;

    float deltay = enemy->v.origin[Y_AX] - actor->v.origin[Y_AX];
    if (deltay < -10)       orient[Z_AX] = 270;
    else if (deltay > 10)   orient[Z_AX] = 90;
    else                    orient[Z_AX] = DI_NODIR;

    // try direct route
    if ((orient[Y_AX] != DI_NODIR) &&
        (orient[Z_AX] != DI_NODIR)
        ) {
        float tdir;
        if (orient[Y_AX] == 0) tdir = orient[Z_AX] == 90 ? 45 : 315;
        else                tdir = orient[Z_AX] == 90 ? 135 : 215;

        if ((tdir != turnaround) &&
            SV_StepDirection(actor, tdir, dist)
            )
            return;
    }

    // try other directions
    if (((rand() & 3) & 1) ||
        (fabs(deltay) > fabs(deltax))
        ) {
        float tdir = orient[Y_AX];
        orient[Y_AX] = orient[Z_AX];
        orient[Z_AX] = tdir;
    }

    if (
        (
            (orient[Y_AX] != DI_NODIR) &&
            (orient[Y_AX] != turnaround) &&
            SV_StepDirection(actor, orient[Y_AX], dist)
            ) ||
        (
            (orient[Z_AX] != DI_NODIR) &&
            (orient[Z_AX] != turnaround) &&
            SV_StepDirection(actor, orient[Z_AX], dist)
            )
        )
        return;

    /* there is no direct path to the player, so pick another direction */

    if ((olddir != DI_NODIR) &&
        SV_StepDirection(actor, olddir, dist)
        )
        return;

    if (rand() & 1) {  /*randomly determine direction of search*/
        for (float tdir = 0; tdir <= 315; tdir += 45)
            if ((tdir != turnaround) &&
                SV_StepDirection(actor, tdir, dist)
                )
                return;
    }
    else {
        for (float tdir = 315; tdir >= 0; tdir -= 45)
            if ((tdir != turnaround) &&
                SV_StepDirection(actor, tdir, dist)
                )
                return;
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
        if ((goal->v.absmin[i] > (ent->v.absmax[i] + dist)) ||
            (goal->v.absmax[i] < (ent->v.absmin[i] - dist)))
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
        G_FLOAT(OFS_RETURN) = 0;
        return;
    }

    // if the next step hits the enemy, return immediately
#ifdef QUAKE2
    edict_p enemy = ED_GetEDictByOffs(ent->v.enemy);
    if ((enemy != Edicts) && SV_CloseEnough(ent, enemy, dist))
#else
    if ((ED_GetEDictByOffs(ent->v.enemy) != Edicts) && SV_CloseEnough(ent, goal, dist))
#endif
        return;

    // bump around...
    if ((rand() & 3) == 1 ||
        !SV_StepDirection(ent, ent->v.ideal_yaw, dist)) {
        SV_NewChaseDir(ent, goal, dist);
    }
}

