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
MoveClipFlags_e SV_FlyMove(edict_p ent, SimDt_t time, trace_p steptrace) {
    MoveClipFlags_e blocked = MOVECLIP_NONE;
    vec3_t original_velocity = ent->v.velocity;
    vec3_t primal_velocity = ent->v.velocity;
    int numplanes = 0;

    SimDt_t time_left = time;

    int numbumps = 4;
    vec3_t planes[MAX_CLIP_PLANES];
    for (int bumpcount = 0; bumpcount < numbumps; bumpcount++) {
        if (!(ent->v.velocity.x) &&
            !(ent->v.velocity.y) &&
            !(ent->v.velocity.z)) break; // ent->v.velocity.is_zero()

        vec3_t end = VectorMA(ent->v.origin, time_left, ent->v.velocity);

        trace_t trace = SV_Move(ent->v.origin, *(BBox_p)&ent->v.mins, end, MOVE_NORMAL, ent
        );

        if (trace.allsolid) { // entity is trapped in another solid
            ent->v.velocity = v3Zero;
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
            ent->v.velocity = v3Zero;
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
                    ent->v.velocity = v3Zero;
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
            ent->v.velocity = v3Zero;
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
        CLAMP(-sv_maxvelocity.value, &ent->v.velocity.v[i], sv_maxvelocity.value);
    }
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

    CLAMP_LESS(&thinktime, (float)SV_GetTime()); // don't let things stay in the past.
    // it is possible to start that way
    // by a trigger with a local time.
    ent->v.nextthink = 0;
    pr_global_struct->time = thinktime;
    pr_global_struct->self = ED_GetEDictOffs(ent);
    pr_global_struct->other = ED_GetEDictOffs(Edicts); // should be 0
    PR_ExecuteProgram(ent->v.think);
    return !ent->free;
}
