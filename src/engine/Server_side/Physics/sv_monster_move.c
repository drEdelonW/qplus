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
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
// static int c_yes, c_no;
#define STEPSIZE (18)

bool SV_CheckBottom(edict_p ent) {
#if 0
    vec3_t mins = VectorAdd(ent->v.origin, ent->v.mins);
    vec3_t maxs = VectorAdd(ent->v.origin, ent->v.maxs);
#else
    BBox_t bb = *(BBox_p)&ent->v.mins;
#endif

    // if all of the points under the corners are solid world, don't bother with the tougher checks
    // the corners must be within 16 of the midpoint
    vec3_t start = v3Zero; {
        start.z = bb.mins.z - 1.f;
    };
    for (int x = 0; x <= 1; x++)
        for (int y = 0; y <= 1; y++) {
            start.x = (x) ? bb.maxs.x : bb.mins.x;
            start.y = (y) ? bb.maxs.y : bb.mins.y;
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
    start = VectorScale(VectorAdd(bb.mins, bb.maxs), 0.5f); {
        start.z = bb.mins.z;   // the midpoint must be within 16 of the bottom
    }

    vec3_t stop = start; {
        stop.z -= TWICE(STEPSIZE);
    }

    trace_t trace = SV_Move(start, bbZero, stop, MOVE_NOMONSTERS, ent);

    if (trace.fraction == 1.f)
        return false;

    float mid = trace.endpos.z;
    float bottom = trace.endpos.z;

    // the corners must be within 16 of the midpoint
    for (int x = 0; x <= 1; x++)
        for (int y = 0; y <= 1; y++) {
            start.x = stop.x = (x) ? bb.maxs.x : bb.mins.x;
            start.y = stop.y = (y) ? bb.maxs.y : bb.mins.y;

            trace = SV_Move(start, bbZero, stop, MOVE_NOMONSTERS, ent);
            if ((trace.fraction != 1.f) &&
                (trace.endpos.z > bottom)
                )   bottom = trace.endpos.z;

            if ((trace.fraction == 1.f) ||
                ((mid - trace.endpos.z) > STEPSIZE)
                )   return false;
        }

    // c_yes++;
    return true;
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
            trace_t trace = SV_Move(ent->v.origin, *(BBox_p)&ent->v.mins, neworg, MOVE_NORMAL, ent);

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

    trace_t trace = SV_Move(neworg, *(BBox_p)&ent->v.mins, end, MOVE_NORMAL, ent);

    if (trace.allsolid)
        return false;

    if (trace.startsolid) {
        neworg.z -= STEPSIZE;
        trace = SV_Move(neworg, *(BBox_p)&ent->v.mins, end, MOVE_NORMAL, ent);
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

