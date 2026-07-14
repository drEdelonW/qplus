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
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition(edict_p pEntIn) {
#ifdef QUAKE2
# if 0
    vec3_t point = {
        pEntIn->v.origin.x,
        pEntIn->v.origin.y,
        pEntIn->v.origin.z + pEntIn->v.mins.z + 1
    };
# else
    vec3_t point = pEntIn->v.origin; {
        point.z += pEntIn->v.mins.z + 1.f;
    }
# endif
    int cont = SV_PointContents(point);
#else
    contents_t cont = SV_PointContents(pEntIn->v.origin);
#endif
    if (!pEntIn->v.watertype) { // just spawned here
        pEntIn->v.watertype = (float)cont;
        pEntIn->v.waterlevel = WL_Feet;
        return;
    }

    if ((contents_t)pEntIn->v.watertype == CONTENTS_EMPTY) // just crossed into water
        SV_StartSound(pEntIn, SndChAuto, "misc/h2ohit1.wav", VolFull, AtnNorm);

    if (cont <= CONTENTS_WATER) {
        pEntIn->v.watertype = (float)cont;
        pEntIn->v.waterlevel = WL_Feet;
    }
    else {
        pEntIn->v.watertype = CONTENTS_EMPTY;
        pEntIn->v.waterlevel = (float)cont;
    }
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/

void SV_Physics_Toss(edict_p pEntIn) {
#ifdef QUAKE2
    edict_p groundentity = ED_GetEDictByOffs(pEntIn->v.groundentity);
    if (groundentity->v.flags & FL_CONVEYOR)    pEntIn->v.basevelocity = VectorScale(groundentity->v.movedir, groundentity->v.speed);
    else                                        pEntIn->v.basevelocity = v3Zero;
    SV_CheckWater(pEntIn);
#endif
    // regular thinking
    if (!SV_RunThink(pEntIn))  return;

#ifdef QUAKE2
    if (pEntIn->v.velocity.z > 0)
        pEntIn->v.flags = (EntityFlags_t)pEntIn->v.flags & ~FL_ONGROUND;

    if (((pEntIn->v.flags & FL_ONGROUND)) &&
        (VectorCompare(pEntIn->v.basevelocity, v3Zero))
        )   return;

    SV_CheckVelocity(pEntIn);

    // add gravity
    if (!(pEntIn->v.flags & FL_ONGROUND) &&
        (pEntIn->v.movetype != MOVETYPE_FLY) &&
        (pEntIn->v.movetype != MOVETYPE_BOUNCEMISSILE) &&
        (pEntIn->v.movetype != MOVETYPE_FLYMISSILE)
        )   SV_AddGravity(pEntIn);

#else
    // if onground, return without moving
    if (((EntityFlags_t)pEntIn->v.flags & FL_ONGROUND))  return;

    SV_CheckVelocity(pEntIn);

    // add gravity
    switch ((movetype_t)pEntIn->v.movetype) {
    case MOVETYPE_FLY:
    case MOVETYPE_FLYMISSILE:   /* no gravity */    break;
    default:                    SV_AddGravity(pEntIn); break;
    }
#endif

    // move angles
    pEntIn->v.angles = AngleMA(pEntIn->v.angles, (float)host_frametime, pEntIn->v.avelocity);
    // move origin
#ifdef QUAKE2
    pEntIn->v.velocity = VectorAdd(pEntIn->v.velocity, pEntIn->v.basevelocity);
#endif
    trace_t trace = SV_PushEntity(pEntIn,
        VectorScale(pEntIn->v.velocity, (float)host_frametime)
    );
#ifdef QUAKE2
    pEntIn->v.velocity = VectorSubtract(pEntIn->v.velocity, pEntIn->v.basevelocity);
#endif
    if ((trace.fraction == 1.f) ||
        (pEntIn->free)
        )   return;

    float backoff;
    switch ((movetype_t)pEntIn->v.movetype) {
#ifdef QUAKE2
    case MOVETYPE_BOUNCEMISSILE:    backoff = 2.f;  break;
#endif
    case MOVETYPE_BOUNCE:           backoff = 1.5f;  break;
    default:                        backoff = 1.f;  break;
    }

    ClipVelocity(pEntIn->v.velocity, trace.plane.normal, &pEntIn->v.velocity, backoff);

    // stop if on ground
    if ((trace.plane.normal.z > 0.7f) &&
        (
            (pEntIn->v.velocity.z < 60.f) ||
            (
#ifdef QUAKE2
            (pEntIn->v.movetype != MOVETYPE_BOUNCEMISSILE) &&
#endif
                (pEntIn->v.movetype != MOVETYPE_BOUNCE)
                )
            )
        ) {
        pEntIn->v.flags = (float)((EntityFlags_t)pEntIn->v.flags | FL_ONGROUND);
        pEntIn->v.groundentity = ED_GetEDictOffs(trace.pEnt);
        pEntIn->v.velocity = v3Zero;
        pEntIn->v.avelocity = a3Zero;

    }

    // check for in water
    SV_CheckWaterTransition(pEntIn);
}



/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None(edict_p pEntIn) { SV_RunThink(pEntIn); } // regular thinking

#ifdef QUAKE2
/*
=============
SV_Physics_Follow

Entities that are "stuck" to another entity
=============
*/
void SV_Physics_Follow(edict_p pEntIn) {
    SV_RunThink(pEntIn);    // regular thinking
    pEntIn->v.origin = VectorAdd(
        ED_GetEDictByOffs(pEntIn->v.aiment)->v.origin, pEntIn->v.v_angle
    );
    SV_LinkEdict(pEntIn, true);
}
#endif




/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip(edict_p pEntIn) {
    // regular thinking
    if (!SV_RunThink(pEntIn))  return;

    pEntIn->v.angles = AngleMA(pEntIn->v.angles, (float)host_frametime, pEntIn->v.avelocity);
    pEntIn->v.origin = VectorMA(pEntIn->v.origin, (float)host_frametime, pEntIn->v.velocity);

    SV_LinkEdict(pEntIn, false);
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
void SV_Physics_Step(edict_p pEntIn) {
    bool hitsound = false;

    edict_p groundentity = ED_GetEDictByOffs(pEntIn->v.groundentity);
    if ((EntityFlags_t)groundentity->v.flags & FL_CONVEYOR)     pEntIn->v.basevelocity = VectorScale(groundentity->v.movedir, groundentity->v.speed);
    else                                                        pEntIn->v.basevelocity = v3Zero;
    //@@
    pGame()->time = SV_GetTime();
    pGame()->self = ED_GetEDictOffs(pEntIn);
    PF_WaterMove();

    SV_CheckVelocity(pEntIn);

    bool wasonground = (EntityFlags_t)pEntIn->v.flags & FL_ONGROUND;
    // pEntIn->v.flags = (EntityFlags_t)pEntIn->v.flags & ~FL_ONGROUND;

        // add gravity except:
        //   flying monsters
        //   swimming monsters who are in the water
    bool inwater = SV_CheckWater(pEntIn);
    if ((!wasonground) &&
        (!((EntityFlags_t)pEntIn->v.flags & FL_FLY)) &&
        (!(
            ((EntityFlags_t)pEntIn->v.flags & FL_SWIM) &&
            (pEntIn->v.waterlevel > WL_None)
            )
            )
        ) {
        hitsound = (pEntIn->v.velocity.z < (sv_gravity.value * -0.1));
        if (!inwater)
            SV_AddGravity(pEntIn);
    }

    if (!VectorCompare(pEntIn->v.velocity, v3Zero) ||
        !VectorCompare(pEntIn->v.basevelocity, v3Zero)
        ) {
        pEntIn->v.flags = (EntityFlags_t)pEntIn->v.flags & ~FL_ONGROUND;
        // apply friction
        // let dead monsters who aren't completely onground slide
        if (wasonground)
            if (!((pEntIn->v.health <= 0.0) &&
                !SV_CheckBottom(pEntIn))
                ) {
                vec3_p vel = pEntIn->v.velocity;
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

        VectorAdd(pEntIn->v.velocity, pEntIn->v.basevelocity, pEntIn->v.velocity);
        SV_FlyMove(pEntIn, host_frametime, NULL);
        pEntIn->v.velocity = VectorSubtract(pEntIn->v.velocity, pEntIn->v.basevelocity);

        // determine if it's on solid ground at all
        {
            BBox_t bb = BBoxTranslate(EvBBox(&pEntIn->v), pEntIn->v.origin);
            vec3_t point = { .z = bb.mins.z - 1.f };
            for (int x = 0; x <= 1; x++)
                for (int y = 0; y <= 1; y++) {
                    point.x = (x) ? bb.maxs.x : bb.mins.x;
                    point.y = (y) ? bb.maxs.y : bb.mins.y;
                    if (SV_PointContents(point) == CONTENTS_SOLID) {
                        pEntIn->v.flags = pEntIn->v.flags | FL_ONGROUND;
                        break;
                    }
                }
        }

        SV_LinkEdict(pEntIn, true);

        if (((EntityFlags_t)pEntIn->v.flags & FL_ONGROUND) &&
            (!wasonground) &&
            (hitsound)
            )   SV_StartSound(pEntIn, SndChAuto, "demon/dland2.wav", VolFull, AtnNorm);
    }

    // regular thinking
    SV_RunThink(pEntIn);
    SV_CheckWaterTransition(pEntIn);
}
#else
void SV_Physics_Step(edict_p pEntIn) {
    // freefall if not onground
    if (!((EntityFlags_t)pEntIn->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM))) {
        bool hitsound = (pEntIn->v.velocity.z < (sv_gravity.value * -0.1f));

        SV_AddGravity(pEntIn);
        SV_CheckVelocity(pEntIn);
        SV_FlyMove(pEntIn, host_frametime, NULL);
        SV_LinkEdict(pEntIn, true);

        if (((EntityFlags_t)pEntIn->v.flags & FL_ONGROUND) && // just hit ground
            (hitsound)
            )   SV_StartSound(pEntIn, SndChAuto, "demon/dland2.wav", VolFull, AtnNorm);
    }

    SV_RunThink(pEntIn);   // regular thinking
    SV_CheckWaterTransition(pEntIn);
}
#endif



#ifndef QUAKE2
trace_t SV_Trace_Toss(edict_p pEntIn, edict_p ignore) {
    LegTime_t save_frametime = host_frametime;
    host_frametime = 0.05;

#warning !!!ACHTUNG!!! sizeof(edict_t) ILLEGAL
#if 0
    edict_t tempent; memcpy(&tempent, pEntIn, sizeof(edict_t));
#else
    edict_t tempent = *pEntIn;
#endif
    edict_p tent = &tempent;

    while (1) {
        SV_CheckVelocity(tent);
        SV_AddGravity(tent);
        tent->v.angles = AngleMA(tent->v.angles, (float)host_frametime, tent->v.avelocity);
        trace_t trace = SV_MoveBox(
            tent->v.origin, VectorMA(tent->v.origin, (float)host_frametime, tent->v.velocity),
            MOVE_NORMAL, tent
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

        if ((trace.pEnt) &&
            (trace.pEnt != ignore)
            )   host_frametime = save_frametime;        // p->color = 224;
        return trace;
    }
}
#endif

