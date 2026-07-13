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
        ent->v.origin[Z_AX] + ent->v.mins[Z_AX] + 1
    };
    int cont = SV_PointContents(point);
#else
    int cont = SV_PointContents(ent->v.origin);
#endif
    if (!ent->v.watertype) { // just spawned here
        ent->v.watertype = (float)cont;
        ent->v.waterlevel = WL_Feet;
        return;
    }

    if ((contents_t)ent->v.watertype == CONTENTS_EMPTY) // just crossed into water
        SV_StartSound(ent, SndChAuto, "misc/h2ohit1.wav", VolFull, AtnNorm);

    if (cont <= CONTENTS_WATER) {
        ent->v.watertype = (float)cont;
        ent->v.waterlevel = WL_Feet;
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
    if ((EntityFlags_t)groundentity->v.flags & FL_CONVEYOR) ent->v.basevelocity = VectorScale(groundentity->v.movedir, groundentity->v.speed);
    else                                                    ent->v.basevelocity = v3Zero;
    SV_CheckWater(ent);
#endif
    // regular thinking
    if (!SV_RunThink(ent))  return;

#ifdef QUAKE2
    if (ent->v.velocity[Z_AX] > 0)
        ent->v.flags = (EntityFlags_t)ent->v.flags & ~FL_ONGROUND;

    if ((((EntityFlags_t)ent->v.flags & FL_ONGROUND)) &&
        (VectorCompare(ent->v.basevelocity, v3Zero)))
        return;

    SV_CheckVelocity(ent);

    // add gravity
    if (!((EntityFlags_t)ent->v.flags & FL_ONGROUND) &&
        (ent->v.movetype != MOVETYPE_FLY) &&
        (ent->v.movetype != MOVETYPE_BOUNCEMISSILE) &&
        (ent->v.movetype != MOVETYPE_FLYMISSILE))
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
    ent->v.angles = AngleMA(ent->v.angles, (float)host_frametime, ent->v.avelocity);
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
        (ent->free)
        )   return;

    float backoff;
    switch ((movetype_t)ent->v.movetype) {
#ifdef QUAKE2
    case MOVETYPE_BOUNCEMISSILE:    backoff = 2.f;  break;
#endif
    case MOVETYPE_BOUNCE:           backoff = 1.5f;  break;
    default:                        backoff = 1.f;  break;
    }

    ClipVelocity(ent->v.velocity, trace.plane.normal, &ent->v.velocity, backoff);

    // stop if on ground
    if ((trace.plane.normal.z > 0.7f) &&
        (
            (ent->v.velocity.z < 60.f) ||
            (
#ifdef QUAKE2
            (ent->v.movetype != MOVETYPE_BOUNCEMISSILE) &&
#endif
                (ent->v.movetype != MOVETYPE_BOUNCE)
                )
            )
        ) {
        ent->v.flags = (float)((int)((EntityFlags_t)ent->v.flags) | FL_ONGROUND);
        ent->v.groundentity = ED_GetEDictOffs(trace.ent);
        ent->v.velocity = v3Zero;
        ent->v.avelocity = a3Zero;

    }

    // check for in water
    SV_CheckWaterTransition(ent);
}



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

    ent->v.angles = AngleMA(ent->v.angles, (float)host_frametime, ent->v.avelocity);
    ent->v.origin = VectorMA(ent->v.origin, (float)host_frametime, ent->v.velocity);

    SV_LinkEdict(ent, false);
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
    else                                                        ent->v.basevelocity = v3Zero;
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
            (ent->v.waterlevel > WL_None)
            ))) {
        hitsound = (ent->v.velocity[Z_AX] < (sv_gravity.value * -0.1));
        if (!inwater)   SV_AddGravity(ent);
    }

    if (!VectorCompare(ent->v.velocity, v3Zero) ||
        !VectorCompare(ent->v.basevelocity, v3Zero)) {
        ent->v.flags = (EntityFlags_t)ent->v.flags & ~FL_ONGROUND;
        // apply friction
        // let dead monsters who aren't completely onground slide
        if (wasonground)
            if (!((ent->v.health <= 0.0) &&
                !SV_CheckBottom(ent))
                ) {
                vec3_p vel = ent->v.velocity;
                float speed = sqrtf((vel->x * vel->x) + (vel->y * vel->y));
                if (speed) {
                    float friction = sv_friction.value;

                    float control = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
                    float newspeed = speed - host_frametime * control * friction;
                    ClampLessThen(&newspeed, 0.f);
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
            BBox_t bb = BBoxTranslate(EvBBox(&ent->v), ent->v.origin);

            vec3_t point = { .z = bb.mins.z - 1.f };
            for (int x = 0; x <= 1; x++)
                for (int y = 0; y <= 1; y++) {
                    point.x = (x) ? bb.maxs.x : bb.mins.x;
                    point.y = (y) ? bb.maxs.y : bb.mins.y;
                    if (SV_PointContents(point) == CONTENTS_SOLID) {
                        ent->v.flags = (EntityFlags_t)ent->v.flags | FL_ONGROUND;
                        break;
                    }
                }
        }

        SV_LinkEdict(ent, true);

        if (((EntityFlags_t)ent->v.flags & FL_ONGROUND) &&
            (!wasonground) &&
            (hitsound)
            )   SV_StartSound(ent, SndChAuto, "demon/dland2.wav", VolFull, AtnNorm);
    }

    // regular thinking
    SV_RunThink(ent);
    SV_CheckWaterTransition(ent);
}
#else
void SV_Physics_Step(edict_p ent) {
    // freefall if not onground
    if (!((EntityFlags_t)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM))) {
        bool hitsound = (ent->v.velocity.z < (sv_gravity.value * -0.1f));

        SV_AddGravity(ent);
        SV_CheckVelocity(ent);
        SV_FlyMove(ent, host_frametime, NULL);
        SV_LinkEdict(ent, true);

        if (((EntityFlags_t)ent->v.flags & FL_ONGROUND) && // just hit ground
            (hitsound)
            )   SV_StartSound(ent, SndChAuto, "demon/dland2.wav", VolFull, AtnNorm);
    }

    SV_RunThink(ent);   // regular thinking
    SV_CheckWaterTransition(ent);
}
#endif



#ifndef QUAKE2
trace_t SV_Trace_Toss(edict_p ent, edict_p ignore) {
    LegTime_t save_frametime = host_frametime;
    host_frametime = 0.05;

    edict_t tempent; memcpy(&tempent, ent, sizeof(edict_t)); // TODO: ACHTUNG! sizeof(edict_t) ILLIGAL
    edict_p tent = &tempent;

    while (1) {
        SV_CheckVelocity(tent);
        SV_AddGravity(tent);
        tent->v.angles = AngleMA(tent->v.angles, (float)host_frametime, tent->v.avelocity);
        vec3_t move = VectorScale(tent->v.velocity, (float)host_frametime);
        vec3_t end = VectorAdd(tent->v.origin, move);
        trace_t trace = SV_Move(tent->v.origin, *(BBox_p)&tent->v.mins, end, MOVE_NORMAL, tent
        );
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
            p->vel = v3Zero;
            p->org = tent->v.origin;
        }
# endif

        if ((trace.ent) &&
            (trace.ent != ignore)
            )   host_frametime = save_frametime;        // p->color = 224;
        return trace;
    }
}
#endif

