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

#include "sv_phys_priv.h"


/*
================
SV_NewChaseDir

================
*/
#define DI_NODIR -1.0f
void SV_NewChaseDir(edict_p actor, edict_p enemy, float dist) {
    // float   orient[3];   // direction angle Euler
    ang3_t  orient;   // direction angle Euler

    float olddir = anglemod((float)((int)(actor->v.ideal_yaw / 45) * 45));
    float turnaround = anglemod(olddir - 180);

    float deltax = enemy->v.origin.x - actor->v.origin.x;
    float deltay = enemy->v.origin.y - actor->v.origin.y;

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
