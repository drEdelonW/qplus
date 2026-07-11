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
// view.c -- player eye positioning


#include "view.h"
#include <math.h>
#include <stdlib.h>  // for atoi()
#include "angle.h"
#include "screen.h"
#include "chase.h"
#include "client.h"
#include "console.h"
#include "cmd.h"
#include "cvar_q1.h"
#include "draw.h"
#include "gamedefs.h"
#include "host.h"
#include "msg.h"
#include "q_tools.h"
#include "transform.h"
#include "render.h"
#include "GameRule.h"
#include "vector_tools.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/


static LegDt_t _v_DmgTime;
static float _v_DmgRoll, _v_DmgPitch;



/*
===============
V_CalcRoll

Used by view and sv_user
===============
*/

static Basis_t _bs;

float V_CalcRoll(ang3_t angles, vec3_t velocity) {
    _bs = GetBasis(angles);
    float side = DotProduct(velocity, _bs.right);
    float sign = (side < 0.f) ? -1.f : 1.f;
    side = fabsf(side);

    float value = cl_rollangle.value;
    // if (cl.inwater)
    //  value *= 6.0f;

    if (side < cl_rollspeed.value)  side = value * (side / cl_rollspeed.value);
    else                            side = value;

    return side * sign;
}


/*
===============
V_CalcBob

===============
*/
float V_CalcBob() {
    LegDt_t cycle = (GetClSimTime() - (int)(GetClSimTime() / cl_bobcycle.value) * cl_bobcycle.value) / cl_bobcycle.value;

    if (cycle < cl_bobup.value) cycle = M_PI * (cycle / cl_bobup.value);
    else                        cycle = M_PI + M_PI * (cycle - cl_bobup.value) / (1.f - cl_bobup.value);

    // bob is proportional to velocity in the xy plane
    // (don't count Z, or jumping messes it up)

    float bob = sqrtf(
        (cl.velocity.x * cl.velocity.x) +
        (cl.velocity.y * cl.velocity.y)
    ) * cl_bob.value;
    //Con_Printf ("speed: %5.1f\n", Length(cl.velocity));
    bob = (bob * 0.3f) + (bob * 0.7f * sinf(cycle));
    CLAMP(-7.f, &bob, 4.f);
    return bob;
}


//=============================================================================

void V_StartPitchDrift() {
#if 1
    if (cl.laststop == GetClSimTime()) return;  // something else is keeping it from drifting
#endif
    if (
        cl.nodrift ||
        !cl.pitchvel
        ) {
        cl.pitchvel = v_centerspeed.value;
        cl.nodrift = false;
        cl.driftmove = 0.f;
    }
}

void V_StopPitchDrift() {
    cl.laststop = GetClSimTime();
    cl.nodrift = true;
    cl.pitchvel = 0.f;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards cl.idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

Drifting is enabled when the center view key is hit, mlook is released and
lookspring is non 0, or when
===============
*/
void V_DriftPitch() {
    if (noclip_anglehack ||
        !cl.onground ||
        cls.isDemoPlaying
        ) {
        cl.driftmove = 0.f;
        cl.pitchvel = 0.f;
        return;
    }

    // don't count small mouse motion
    if (cl.nodrift) {
        if (fabs(cl.cmd.move.forward) < cl_forwardspeed.value)  cl.driftmove = 0.f;
        else                                                    cl.driftmove += host_frametime;

        if (cl.driftmove > v_centermove.value)      V_StartPitchDrift();
        return;
    }

    float delta = cl.idealpitch - cl.viewangles.pitch;

    if (!delta) {
        cl.pitchvel = 0.f;
        return;
    }

    float move = host_frametime * cl.pitchvel;
    cl.pitchvel += host_frametime * v_centerspeed.value;

    //Con_Printf ("move: %f (%f)\n", move, host_frametime);

    /**/ if (delta > 0.f) {
        if (move > delta) {
            cl.pitchvel = 0.f;
            move = delta;
        }
        cl.viewangles.pitch += move;
    }
    else if (delta < 0.f) {
        if (move > -delta) {
            cl.pitchvel = 0.f;
            move = -delta;
        }
        cl.viewangles.pitch -= move;
    }
}


/*
==============================================================================

                        PALETTE FLASHES

==============================================================================
*/


ColorShift_t cshift_empty = { {130, 80, 50}, 0 };
ColorShift_t cshift_water = { {130, 80, 50}, 128 };
ColorShift_t cshift_slime = { {0, 25, 5}, 150 };
ColorShift_t cshift_lava = { {255, 80, 0}, 150 };

uint8_t  gammatable[InksNum]; // palette is sent through this


void BuildGammaTable(float g) {
    if (g == 1.0f) {
        for (int i = 0; i < InksNum; i++)
            gammatable[i] = (uint8_t)i;
        return;
    }

    for (int i = 0; i < InksNum; i++) {
        int inf = 255 * pow((i + 0.5f) / 255.5, g) + 0.5f;
        CLAMP(0, &inf, 255);
        gammatable[i] = (uint8_t)inf;
    }
}

/*
=================
V_CheckGamma
=================
*/
bool V_CheckGamma() {
    static float _oldGammaValue;
    if (v_gamma.value == _oldGammaValue)     return false;
    _oldGammaValue = v_gamma.value;

    BuildGammaTable(v_gamma.value);
    SCR_RequestCalcRefdef();    // force a surface cache flush

    return true;
}



/*
===============
V_ParseDamage
===============
*/
void V_ParseDamage() {
    int armor = MSG_ReadByte();
    int blood = MSG_ReadByte();
    vec3_t from = MSG_ReadVector();

    float count = blood * 0.5f + armor * 0.5f;
    CLAMP_LESS(&count, 10.f);

    cl.faceanimtime = GetClSimTime() + 0.2f;  // but sbar face into pain frame

    cl.cshifts[CSHIFT_DAMAGE].percent += 3 * count;
    CLAMP(0, &cl.cshifts[CSHIFT_DAMAGE].percent, 150);  // for x86 must be signed and more then 8bit

    if (armor > blood) {
        cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
        cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
        cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
    }
    else if (armor) {
        cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
        cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
        cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
    }
    else {
        cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
        cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
        cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
    }

    //
    // calculate view angle kicks
    //
    r_Entity_p ent = &cl_entities[cl.viewentity];

    from = VectorSubtract(from, ent->pose.spot);
    VectorNormalize(&from);

    _bs = GetBasis(ent->pose.facing);

    _v_DmgRoll = count * v_kickroll.value * DotProduct(from, _bs.right);
    _v_DmgPitch = count * v_kickpitch.value * DotProduct(from, _bs.forward);

    _v_DmgTime = v_kicktime.value;
}


/*
==================
V_cshift_f
==================
*/
void V_cshift_f() {
    cshift_empty = (ColorShift_t){
        {
            atoi(Cmd_Argv(1)),
            atoi(Cmd_Argv(2)),
            atoi(Cmd_Argv(3))
        },
        atoi(Cmd_Argv(4))
    };
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
void V_BonusFlash_f() {
    cl.cshifts[CSHIFT_BONUS] = (ColorShift_t){ {215, 186, 69}, 50 };
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
void V_SetContentsColor(contents_t contents) {
    switch (contents) {
    case CONTENTS_SOLID:
    case CONTENTS_EMPTY:    cl.cshifts[CSHIFT_CONTENTS] = cshift_empty; break;
    case CONTENTS_LAVA:     cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;  break;
    case CONTENTS_SLIME:    cl.cshifts[CSHIFT_CONTENTS] = cshift_slime; break;
    default:                cl.cshifts[CSHIFT_CONTENTS] = cshift_water;
    }
}

/*
=============
V_CalcPowerupCshift
=============
*/
void V_CalcPowerupCshift() {
#if 0
    if (cl.items & IT_QUAD) {
        cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
        cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
        cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
        cl.cshifts[CSHIFT_POWERUP].percent = 30;
    }
    else if (cl.items & IT_SUIT) {
        cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
        cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
        cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
        cl.cshifts[CSHIFT_POWERUP].percent = 20;
    }
    else if (cl.items & IT_INVISIBILITY) {
        cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
        cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
        cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
        cl.cshifts[CSHIFT_POWERUP].percent = 100;
    }
    else if (cl.items & IT_INVULNERABILITY) {
        cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
        cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
        cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
        cl.cshifts[CSHIFT_POWERUP].percent = 30;
    }
    else
        cl.cshifts[CSHIFT_POWERUP].percent = 0;
#else
    /* */if (cl.items & IT_QUAD)            cl.cshifts[CSHIFT_POWERUP] = (ColorShift_t){ {0, 0, 50}, 30 };
    else if (cl.items & IT_SUIT)            cl.cshifts[CSHIFT_POWERUP] = (ColorShift_t){ {0, 255, 0}, 20 };
    else if (cl.items & IT_INVISIBILITY)    cl.cshifts[CSHIFT_POWERUP] = (ColorShift_t){ {100, 100, 100}, 100 };
    else if (cl.items & IT_INVULNERABILITY) cl.cshifts[CSHIFT_POWERUP] = (ColorShift_t){ {255, 255, 0}, 30 };
    else                                    cl.cshifts[CSHIFT_POWERUP].percent = 0;
#endif
}

/*
=============
V_CalcBlend
=============
*/
#ifdef GLQUAKE
float  v_blend[4];  // rgba 0.0 - 1.0

void V_CalcBlend() {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;
    float byteScaleFactor = 1.0f / 255.0f;

    for (int IdxShClr = 0; IdxShClr < NUM_CSHIFTS; IdxShClr++) {
        if (!gl_cshiftpercent.value)
            continue;

        float a2 = ((cl.cshifts[IdxShClr].percent * gl_cshiftpercent.value) / 100.0) * byteScaleFactor;

        //  a2 = cl.cshifts[IdxShClr].percent * byteScaleFactor;
        if (!a2)
            continue;
        a = a + a2 * (1 - a);
        //Con_Printf ("IdxShClr:%i a:%f\n", IdxShClr, a);
        a2 = a2 / a;
        r = r * (1 - a2) + cl.cshifts[IdxShClr].destcolor[0] * a2;
        g = g * (1 - a2) + cl.cshifts[IdxShClr].destcolor[1] * a2;
        b = b * (1 - a2) + cl.cshifts[IdxShClr].destcolor[2] * a2;
    }

    v_blend[0] = r * byteScaleFactor;
    v_blend[1] = g * byteScaleFactor;
    v_blend[2] = b * byteScaleFactor;
    v_blend[3] = a;

    CLAMP(0.f, &v_blend[3], 1.f);
}
#endif

/*
=============
V_UpdatePalette
=============
*/
#ifdef GLQUAKE
qPal_t  ramps;

void V_UpdatePalette() {
    V_CalcPowerupCshift();
    bool new = false;

    for (int IdxShClr = 0; IdxShClr < NUM_CSHIFTS; IdxShClr++) {
        if (cl.cshifts[IdxShClr].percent != cl.prev_cshifts[IdxShClr].percent) {
            new = true;
            cl.prev_cshifts[IdxShClr].percent = cl.cshifts[IdxShClr].percent;
        }
        for (int IdxDstClr = 0; IdxDstClr < 3; IdxDstClr++)
            if (cl.cshifts[IdxShClr].destcolor[IdxDstClr] != cl.prev_cshifts[IdxShClr].destcolor[IdxDstClr]) {
                new = true;
                cl.prev_cshifts[IdxShClr].destcolor[IdxDstClr] = cl.cshifts[IdxShClr].destcolor[IdxDstClr];
            }
    }

    // drop the damage value
    cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime * 150;
    CLAMP_LESS(&cl.cshifts[CSHIFT_DAMAGE].percent, 0);

    // drop the bonus value
    cl.cshifts[CSHIFT_BONUS].percent -= host_frametime * 100;
    CLAMP_LESS(&cl.cshifts[CSHIFT_BONUS].percent, 0);

    bool force = V_CheckGamma();
    if (!new && !force)
        return;
    //-------------------
    V_CalcBlend();

    float a = v_blend[3];
    float r = 255 * v_blend[0] * a;
    float g = 255 * v_blend[1] * a;
    float b = 255 * v_blend[2] * a;

    a = 1 - a;
    for (int i = 0; i < 256; i++) {
        int ir = i * a + r;
        int ig = i * a + g;
        int ib = i * a + b;
        CLAMP_MORE(&ir, 255);
        CLAMP_MORE(&ig, 255);
        CLAMP_MORE(&ib, 255);

        ramps.ink[i].r = gammatable[ir];
        ramps.ink[i].g = gammatable[ig];
        ramps.ink[i].b = gammatable[ib];
    }

    qPal_t pal;

    for (int i = 0; i < InksNum; i++) {
        int ir = host_basepal->ink[i].r;
        int ig = host_basepal->ink[i].g;
        int ib = host_basepal->ink[i].b;

        pal.ink[i].r = ramps.ink[ir].r;
        pal.ink[i].g = ramps.ink[ig].g;
        pal.ink[i].b = ramps.ink[ib].b;
        //-------------------
    }

    VID_ShiftPalette(&pal);
}
#else // !GLQUAKE
void V_UpdatePalette() {

    V_CalcPowerupCshift();

    bool new = false;

    for (int IdxShClr = 0; IdxShClr < NUM_CSHIFTS; IdxShClr++) {
        if (cl.cshifts[IdxShClr].percent != cl.prev_cshifts[IdxShClr].percent) {
            new = true;
            cl.prev_cshifts[IdxShClr].percent = cl.cshifts[IdxShClr].percent;
        }
        for (int IdxDstClr = 0; IdxDstClr < 3; IdxDstClr++)
            if (cl.cshifts[IdxShClr].destcolor[IdxDstClr] != cl.prev_cshifts[IdxShClr].destcolor[IdxDstClr]) {
                new = true;
                cl.prev_cshifts[IdxShClr].destcolor[IdxDstClr] = cl.cshifts[IdxShClr].destcolor[IdxDstClr];
            }
    }

    // drop the damage value
    cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime * 150;
    CLAMP_LESS(&cl.cshifts[CSHIFT_DAMAGE].percent, 0);

    // drop the bonus value
    cl.cshifts[CSHIFT_BONUS].percent -= host_frametime * 100;
    CLAMP_LESS(&cl.cshifts[CSHIFT_BONUS].percent, 0);

    bool force = V_CheckGamma();
    if (!new && !force)
        return;
    //-------------------
    qPal_t pal;
    for (int i = 0; i < InksNum; i++) {
        qRgb24 col = host_basepal->ink[i];

        for (int IdxShClr = 0; IdxShClr < NUM_CSHIFTS; IdxShClr++) {
            col.r += (cl.cshifts[IdxShClr].percent * (cl.cshifts[IdxShClr].destcolor[0] - col.r)) >> 8;
            col.g += (cl.cshifts[IdxShClr].percent * (cl.cshifts[IdxShClr].destcolor[1] - col.g)) >> 8;
            col.b += (cl.cshifts[IdxShClr].percent * (cl.cshifts[IdxShClr].destcolor[2] - col.b)) >> 8;
        }

        pal.ink[i].r = gammatable[col.r];
        pal.ink[i].g = gammatable[col.g];
        pal.ink[i].b = gammatable[col.b];
        //-------------------
    }

    VID_ShiftPalette(&pal);
}
#endif // !GLQUAKE


/*
==============================================================================

                        VIEW RENDERING

==============================================================================
*/

/*
==================
CalcGunAngle
==================
*/
static ang3_t _old = { .pitch = 0.f, .yaw = 0.f };
void CalcGunAngle() {
    ang3_t cAngl = {
        .pitch = -r_refdef.view.facing.pitch,
        .yaw = r_refdef.view.facing.yaw,
    };
    // TODO: solve this puzzle
    // float move = host_frametime * 20.f;
    float pitch = angledelta((_old.pitch - cAngl.pitch)) * 0.4f; // vertical
    CLAMP(-10.f, &pitch, 10.f);
    // if (cAngl.pitch > _old.pitch)    CLAMP_MORE(&pitch, (_old.pitch + move));
    // else                             CLAMP_LESS(&pitch, (_old.pitch - move));

    float yaw = angledelta(_old.yaw - cAngl.yaw) * 0.4f; // horizontal
    CLAMP(-10.f, &yaw, 10.f);
    // if (cAngl.yaw > _old.yaw)    CLAMP_MORE(&yaw, (_old.yaw + move));
    // else                         CLAMP_LESS(&yaw, (_old.yaw - move));

    // Con_Printf("p:%f y:%f\t p:%f y:%f\t \n",
    //     cAngl.pitch, cAngl.yaw,
    //     pitch, yaw
    // );
    _old = cAngl;

    cl.viewent.pose.facing.pitch = -(r_refdef.view.facing.pitch + pitch);
    cl.viewent.pose.facing.yaw = r_refdef.view.facing.yaw + yaw;
    // cl.viewent.pose.facing.roll = r_refdef.viewangles.roll + yaw * 2;`

    cl.viewent.pose.facing.roll -= v_idlescale.value * sinf(GetClSimTime() * v_iroll_cycle.value) * v_iroll_level.value;
    cl.viewent.pose.facing.pitch -= v_idlescale.value * sinf(GetClSimTime() * v_ipitch_cycle.value) * v_ipitch_level.value;
    cl.viewent.pose.facing.yaw -= v_idlescale.value * sinf(GetClSimTime() * v_iyaw_cycle.value) * v_iyaw_level.value;
}

/*
==============
V_BoundOffsets
==============
*/
void V_BoundOffsets() {
    r_Entity_p ent = &cl_entities[cl.viewentity];

    // absolutely bound refresh reletive to entity clipping hull
    // so the view can never be inside a solid wall

    CLAMP(ent->pose.spot.x - 14.f, &r_refdef.view.spot.x, ent->pose.spot.x + 14.f);
    CLAMP(ent->pose.spot.y - 14.f, &r_refdef.view.spot.y, ent->pose.spot.y + 14.f);
    CLAMP(ent->pose.spot.z - 22.f, &r_refdef.view.spot.z, ent->pose.spot.z + 30.f);
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle() {
    ang3_t v_i = (ang3_t){
        .pitch = sinf(GetClSimTime() * v_ipitch_cycle.value) * v_ipitch_level.value,
        .yaw = sinf(GetClSimTime() * v_iyaw_cycle.value) * v_iyaw_level.value,
        .roll = sinf(GetClSimTime() * v_iroll_cycle.value) * v_iroll_level.value,
    };
    r_refdef.view.facing = AngleMA(r_refdef.view.facing, v_idlescale.value, v_i);
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll() {
    float side = V_CalcRoll(cl_entities[cl.viewentity].pose.facing, cl.velocity);
    r_refdef.view.facing.roll += side;

    if (_v_DmgTime > 0.0f) {
        r_refdef.view.facing.roll += _v_DmgTime / v_kicktime.value * _v_DmgRoll;
        r_refdef.view.facing.pitch += _v_DmgTime / v_kicktime.value * _v_DmgPitch;
        _v_DmgTime -= host_frametime;
    }

    if (cl.stats[STAT_HEALTH] <= 0.0f) {
        r_refdef.view.facing.roll = 80.0f; // dead view angle
        return;
    }

}


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef() {
    r_Entity_p ent = &cl_entities[cl.viewentity];    // ent is the player model (visible when out of body)
    r_Entity_p view = &cl.viewent;    // view is the weapon model (only visible from inside body)

    r_refdef.view.spot = ent->pose.spot;
    r_refdef.view.facing = ent->pose.facing;
    view->model = NULL;

    // allways idle in intermission
    float old = v_idlescale.value;
    v_idlescale.value = 1.f;
    V_AddIdle();
    v_idlescale.value = old;
}

/*
==================
V_CalcRefdef

==================
*/
#include "vid.h"    // vid.colormap
void V_CalcRefdef() {
    static float _oldZ = 0.f;

    V_DriftPitch();
    r_Entity_p ent = &cl_entities[cl.viewentity];   // ent is the player model (visible when out of body)
    r_Entity_p view = &cl.viewent;  // view is the weapon model (only visible from inside body)


    // transform the view offset by the model's matrix to get the offset from model origin for the view
    ent->pose.facing.yaw = cl.viewangles.yaw; // the model should face the view dir
    ent->pose.facing.pitch = -cl.viewangles.pitch; // the model should face the view dir


    float bob = V_CalcBob();
    // refresh position
    r_refdef.view.spot = ent->pose.spot;
    r_refdef.view.spot.z += cl.viewheight + bob;

    // never let it sit exactly on a node line, because a water plane can
    // dissapear when viewed with the eye exactly on it.
    // the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
    r_refdef.view.spot = VectorAdd(r_refdef.view.spot, Scalar2Vector(1.0 / 32));

    r_refdef.view.facing = cl.viewangles;
    V_CalcViewRoll();
    V_AddIdle();

    ang3_t angles = ent->pose.facing; {
        angles.pitch = -angles.pitch;   // because entity pitches are actually backward
    }
    _bs = GetBasis(angles);

    r_refdef.view.spot =
        VectorMA(VectorMA(VectorMA(r_refdef.view.spot,
            scr_ofsz.value, _bs.up),
            scr_ofsy.value, _bs.right),
            scr_ofsx.value, _bs.forward
        );


    V_BoundOffsets();

    view->pose.facing = cl.viewangles;    // set up gun position

    CalcGunAngle();

    view->pose.spot = ent->pose.spot; {
        view->pose.spot.z += cl.viewheight;
    }

    view->pose.spot = VectorMA(view->pose.spot, bob * 0.4f, _bs.forward);
    // view->pose.spot = VectorMA(view->pose.spot, bob * 0.4f, _bs.right);
    // view->pose.spot = VectorMA(view->pose.spot, bob * 0.8f, _bs.up);
    view->pose.spot.z += bob;

    // fudge position around to keep amount of weapon visible
    // roughly equal with different FOV

#if 0
    if (cl.model_precache[cl.stats[STAT_WEAPON]] && strcmp(cl.model_precache[cl.stats[STAT_WEAPON]]->name, "progs/v_shot2.mdl")) {}
#endif
    /**/ if (scr_viewsize.value == 110) view->pose.spot.z += 1;
    else if (scr_viewsize.value == 100) view->pose.spot.z += 2;
    else if (scr_viewsize.value == 90)  view->pose.spot.z += 1;
    else if (scr_viewsize.value == 80)  view->pose.spot.z += 0.5;

    view->model = cl.model_precache[cl.stats[STAT_WEAPON]];
    view->frame = cl.stats[STAT_WEAPONFRAME];
    view->colormap = vid.colormap;

    // set up the refresh position
    r_refdef.view.facing = AngleAdd(r_refdef.view.facing, cl.punchangle);

    // smooth out stair step ups
    if ((cl.onground) &&
        ((ent->pose.spot.z - _oldZ) > 0.f)) {

        LegDt_t steptime = GetClSimTime() - cl.oldtime;
        CLAMP_LESS(&steptime, 0.f); //FIXME  I_Error ("steptime < 0");

        _oldZ += steptime * 80.f;
        CLAMP_MORE(&_oldZ, ent->pose.spot.z);
        if ((ent->pose.spot.z - _oldZ) > 12.f)     _oldZ = ent->pose.spot.z - 12.f;
        r_refdef.view.spot.z += _oldZ - ent->pose.spot.z;
        view->pose.spot.z += _oldZ - ent->pose.spot.z;
    }
    else { _oldZ = ent->pose.spot.z; }

    if (chase_active.value)
        Chase_Update();
}

/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/

void V_RenderView() {
    if (con.forcedup)   return;

    // don't allow cheats in multiplayer
    if (cl.maxclients > 1) {
        Cvar_Set("scr_ofsx", "0");
        Cvar_Set("scr_ofsy", "0");
        Cvar_Set("scr_ofsz", "0");
    }

    if (isIntermission()) { // intermission / finale rendering
        V_CalcIntermissionRefdef();
    }
    else {
        if (!cl.paused /* &&
            (
                (isMultiplayer()) ||
                (key.dest == key_game)) */
            )   V_CalcRefdef();
    }

    R_PushDlights();

#ifndef STM32
    if (lcd_x.value) {
        //
        // render two interleaved views
        //

        Scr.vrect.rowBytes = TWICE(Scr.vrect.rowBytes);
        Scr.vpAspect *= 0.5f;

        r_refdef.view.facing.yaw -= lcd_yaw.value;
        r_refdef.view.spot = VectorMA(r_refdef.view.spot, -lcd_x.value, _bs.right);
        R_RenderView();
        vid.frameBuff.pBuff += HALF(Scr.vrect.rowBytes);

        R_PushDlights();

        r_refdef.view.facing.yaw += lcd_yaw.value * 2.0f;

        r_refdef.view.spot = VectorMA(r_refdef.view.spot, lcd_x.value * 2.0f, _bs.right);
        R_RenderView();
        vid.frameBuff.pBuff -= HALF(Scr.vrect.rowBytes);

        r_refdef.vrect.height = TWICE(r_refdef.vrect.height);

        Scr.vrect.rowBytes = HALF(Scr.vrect.rowBytes);
        Scr.vpAspect *= 2.f;
    }
    else
#endif
        R_RenderView();

    HUD_crosshair();
}


void HUD_crosshair() {
    if (crosshair.value)
        Draw_Character(
#if GLQUAKE
            r_refdef.vrect.x + HALF(r_refdef.vrect.width),
            r_refdef.vrect.y + HALF(r_refdef.vrect.height),
#else
            r_refdef.vrect.x + HALF(r_refdef.vrect.width) + cl_crossx.value,
            r_refdef.vrect.y + HALF(r_refdef.vrect.height) + cl_crossy.value,
#endif
            '+'
        );
}

/*
=================
V_SizeUp_f

Keybinding command
=================
*/
void V_SizeUp_f() {
    Cvar_SetValue("viewsize", scr_viewsize.value + 10.f);
    SCR_RequestCalcRefdef();
}


/*
=================
V_SizeDown_f

Keybinding command
=================
*/
void V_SizeDown_f() {
    Cvar_SetValue("viewsize", scr_viewsize.value - 10.f);
    SCR_RequestCalcRefdef();
}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init() {
    Cmd_AddCommand("v_cshift", V_cshift_f);
    Cmd_AddCommand("bf", V_BonusFlash_f);
    Cmd_AddCommand("centerview", V_StartPitchDrift);

    Cmd_AddCommand("sizeup", V_SizeUp_f);
    Cmd_AddCommand("sizedown", V_SizeDown_f);

#ifndef STM32
    Cvar_RegisterVariable(&lcd_x);
    Cvar_RegisterVariable(&lcd_yaw);
#endif

    Cvar_RegisterVariable(&v_centermove);
    Cvar_RegisterVariable(&v_centerspeed);

    Cvar_RegisterVariable(&v_iyaw_cycle);
    Cvar_RegisterVariable(&v_iroll_cycle);
    Cvar_RegisterVariable(&v_ipitch_cycle);
    Cvar_RegisterVariable(&v_iyaw_level);
    Cvar_RegisterVariable(&v_iroll_level);
    Cvar_RegisterVariable(&v_ipitch_level);

    Cvar_RegisterVariable(&v_idlescale);
    Cvar_RegisterVariable(&crosshair);
    Cvar_RegisterVariable(&cl_crossx);
    Cvar_RegisterVariable(&cl_crossy);
    Cvar_RegisterVariable(&gl_cshiftpercent);

    Cvar_RegisterVariable(&scr_ofsx);
    Cvar_RegisterVariable(&scr_ofsy);
    Cvar_RegisterVariable(&scr_ofsz);

    Cvar_RegisterVariable(&cl_rollspeed);
    Cvar_RegisterVariable(&cl_rollangle);
    Cvar_RegisterVariable(&cl_bob);
    Cvar_RegisterVariable(&cl_bobcycle);
    Cvar_RegisterVariable(&cl_bobup);

    Cvar_RegisterVariable(&v_kicktime);
    Cvar_RegisterVariable(&v_kickroll);
    Cvar_RegisterVariable(&v_kickpitch);
    Cvar_RegisterVariable(&scr_fov);


    BuildGammaTable(1.f); // no gamma yet
    Cvar_RegisterVariable(&v_gamma);
}


