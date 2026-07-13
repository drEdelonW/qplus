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
                ent->v.origin = VectorAdd(
                    org,
                    (vec3_t) {
                        .x = (float)i,
                        .y = (float)j,
                        .z = (float)z,
                    }
                );

                if (!SV_TestEntityPosition(ent)) {
                    Con_DPrintf("Unstuck.\n");
                    SV_LinkEdict(ent, true);
                    return;
                }
            }

    ent->v.origin = org;
    Con_DPrintf("player is stuck.\n");
}


bool SV_CheckWater(edict_p ent) {
    vec3_t point = ent->v.origin; {
        point.z += ent->v.mins.z + 1.f;
    };

    ent->v.waterlevel = WL_None;
    ent->v.watertype = CONTENTS_EMPTY;
    contents_t cont = SV_PointContents(point);
    if (cont <= CONTENTS_WATER) {
#ifdef QUAKE2
        contents_t truecont = SV_TruePointContents(point);
#endif
        ent->v.watertype = cont;
        ent->v.waterlevel = WL_Feet;
        point.z = ent->v.origin.z + (ent->v.mins.z + ent->v.maxs.z) * 0.5f;
        cont = SV_PointContents(point);
        if (cont <= CONTENTS_WATER) {
            ent->v.waterlevel = WL_Waist;
            point.z = ent->v.origin.z + ent->v.view_ofs.z;
            cont = SV_PointContents(point);
            if (cont <= CONTENTS_WATER)
                ent->v.waterlevel = WL_Head;
        }
#ifdef QUAKE2
        if ((truecont <= CONTENTS_CURRENT_0) &&
            (truecont >= CONTENTS_CURRENT_DOWN)
            ) {
            static vec3_t current_table[] = {
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

    return ent->v.waterlevel > WL_Feet;
}


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
    vec3_t dir = v3Zero;

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

    ent->v.velocity = v3Zero;
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
        !(SV_FlyMove(ent, host_frametime, &steptrace) & MOVECLIP_WALL) ||   // move didn't block on a step
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

    vec3_t downmove = (vec3_t){ .z = (float)(-STEPSIZE + oldvel.z * host_frametime) };

    vec3_t upmove = (vec3_t){ .z = STEPSIZE };    // move up
    SV_PushEntity(ent, upmove); // FIXME: don't link?

    // move forward
    ent->v.velocity = (vec3_t){
        .x = oldvel.x,
        .y = oldvel.y
    };

    MoveClipFlags_e clip = SV_FlyMove(ent, host_frametime, &steptrace);

    // check for stuckness, possibly due to the limited precision of floats
    // in the clipping hulls
    if (clip) {
        if ((fabsf(oldorg.y - ent->v.origin.y) < DIST_EPSILON) &&
            (fabsf(oldorg.x - ent->v.origin.x) < DIST_EPSILON)    // stepping up didn't make any progress
            )   clip = SV_TryUnstick(ent, oldvel);
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
void SV_Physics_Client(edict_p ent, EdIdx clNum) {
    if (!svs.clients[clNum - EdictPlayer1].active)   return;  // unconnected slot

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
    case MOVETYPE_FLY:      SV_FlyMove(ent, host_frametime, NULL);  break;
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
