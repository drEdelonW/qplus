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

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/


static float _v_DmgTime;
static float _v_DmgRoll, _v_DmgPitch;



/*
===============
V_CalcRoll

Used by view and sv_user
===============
*/

static Basis_t _bs;

float V_CalcRoll(vec3_t angles, vec3_t velocity) {
    _bs = GetBasis(angles);
    float side = DotProduct(velocity, _bs.right);
    float sign = (side < 0.0f) ? -1.0f : 1.0f;
    side = fabs(side);

    float value = cl_rollangle.value;
    // if (cl.inwater)
    //  value *= 6.0f;

    if (side < cl_rollspeed.value)  side = side * value / cl_rollspeed.value;
    else                            side = value;

    return side * sign;
}


/*
===============
V_CalcBob

===============
*/
float V_CalcBob() {
    float cycle = cl.time - (int)(cl.time / cl_bobcycle.value) * cl_bobcycle.value;
    cycle /= cl_bobcycle.value;

    if (cycle < cl_bobup.value) cycle = M_PI * cycle / cl_bobup.value;
    else                        cycle = M_PI + M_PI * (cycle - cl_bobup.value) / (1.0 - cl_bobup.value);

    // bob is proportional to velocity in the xy plane
    // (don't count Z, or jumping messes it up)

    float bob = sqrt(
        (cl.velocity.x * cl.velocity.x) +
        (cl.velocity.y * cl.velocity.y)
    ) * cl_bob.value;
    //Con_Printf ("speed: %5.1f\n", Length(cl.velocity));
    bob = bob * 0.3 + bob * 0.7 * sin(cycle);
    if (bob > 4)        bob = 4;
    else if (bob < -7)  bob = -7;

    return bob;
}


//=============================================================================

void V_StartPitchDrift() {
#if 1
    if (cl.laststop == cl.time) return;  // something else is keeping it from drifting
#endif
    if (
        cl.nodrift ||
        !cl.pitchvel
        ) {
        cl.pitchvel = v_centerspeed.value;
        cl.nodrift = false;
        cl.driftmove = 0;
    }
}

void V_StopPitchDrift() {
    cl.laststop = cl.time;
    cl.nodrift = true;
    cl.pitchvel = 0;
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
        cls.demoplayback
        ) {
        cl.driftmove = 0;
        cl.pitchvel = 0;
        return;
    }

    // don't count small mouse motion
    if (cl.nodrift) {
        if (fabs(cl.cmd.forwardmove) < cl_forwardspeed.value)   cl.driftmove = 0;
        else                                                    cl.driftmove += host_frametime;

        if (cl.driftmove > v_centermove.value)
            V_StartPitchDrift();
        return;
    }

    float delta = cl.idealpitch - cl.viewangles.pitch;

    if (!delta) { cl.pitchvel = 0; return; }

    float move = host_frametime * cl.pitchvel;
    cl.pitchvel += host_frametime * v_centerspeed.value;

    //Con_Printf ("move: %f (%f)\n", move, host_frametime);

    if (delta > 0) {
        if (move > delta) {
            cl.pitchvel = 0;
            move = delta;
        }
        cl.viewangles.pitch += move;
    }
    else if (delta < 0) {
        if (move > -delta) {
            cl.pitchvel = 0;
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


uint8_t  gammatable[256]; // palette is sent through this



void BuildGammaTable(float g) {
    if (g == 1.0f) {
        for (int i = 0; i < 256; i++)
            gammatable[i] = (uint8_t)i;
        return;
    }

    for (int i = 0; i < 256; i++) {
        int inf = 255 * pow((i + 0.5f) / 255.5, g) + 0.5f;
        CLAMP(0, inf, 255);
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
    vec3_t from = {
        .x = MSG_ReadCoord(),
        .y = MSG_ReadCoord(),
        .z = MSG_ReadCoord()
    };

    float count = blood * 0.5f + armor * 0.5f;
    if (count < 10)
        count = 10;

    cl.faceanimtime = cl.time + 0.2f;  // but sbar face into pain frame

    cl.cshifts[CSHIFT_DAMAGE].percent += 3 * count;
    // if (cl.cshifts[CSHIFT_DAMAGE].percent < 0)
    //     cl.cshifts[CSHIFT_DAMAGE].percent = 0;
    if (cl.cshifts[CSHIFT_DAMAGE].percent > 150)
        cl.cshifts[CSHIFT_DAMAGE].percent = 150;

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

    from = VectorSubtract(from, ent->origin);
    VectorNormalize(&from);

    _bs = GetBasis(ent->angles);

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
    case CONTENTS_EMPTY:
    case CONTENTS_SOLID:    cl.cshifts[CSHIFT_CONTENTS] = cshift_empty; break;
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
# if 0
    if (v_blend[3] > 1.0f)        v_blend[3] = 1.0f;
    if (v_blend[3] < 0.0f)        v_blend[3] = 0.0f;
# else
    CLAMP(0.0f, v_blend[3], 1.0f);
# endif
}
#endif

/*
=============
V_UpdatePalette
=============
*/
#ifdef GLQUAKE
uint8_t  ramps[3][256];

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
    if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
        cl.cshifts[CSHIFT_DAMAGE].percent = 0;

    // drop the bonus value
    cl.cshifts[CSHIFT_BONUS].percent -= host_frametime * 100;
    if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
        cl.cshifts[CSHIFT_BONUS].percent = 0;

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
        if (ir > 255)   ir = 255;
        if (ig > 255)   ig = 255;
        if (ib > 255)   ib = 255;

        ramps[0][i] = gammatable[ir];
        ramps[1][i] = gammatable[ig];
        ramps[2][i] = gammatable[ib];
    }

    uint8_t pal[255 * 3];
    uint8_p basepal = host_basepal;
    uint8_p newpal = pal;

    for (int i = 0; i < 256; i++) {
        int ir = basepal[0];
        int ig = basepal[1];
        int ib = basepal[2];
        basepal += 3;

        newpal[0] = ramps[0][ir];
        newpal[1] = ramps[1][ig];
        newpal[2] = ramps[2][ib];
        //-------------------
        newpal += 3;
    }

    VID_ShiftPalette(pal);
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
    if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
        cl.cshifts[CSHIFT_DAMAGE].percent = 0;

    // drop the bonus value
    cl.cshifts[CSHIFT_BONUS].percent -= host_frametime * 100;
    if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
        cl.cshifts[CSHIFT_BONUS].percent = 0;

    bool force = V_CheckGamma();
    if (!new && !force)
        return;
    //-------------------
    uint8_t pal[256 * 3];
    uint8_p basepal = host_basepal;
    uint8_p newpal = pal;

    for (int i = 0; i < 256; i++) {
        int r = basepal[0];
        int g = basepal[1];
        int b = basepal[2];
        basepal += 3;

        for (int IdxShClr = 0; IdxShClr < NUM_CSHIFTS; IdxShClr++) {
            r += (cl.cshifts[IdxShClr].percent * (cl.cshifts[IdxShClr].destcolor[0] - r)) >> 8;
            g += (cl.cshifts[IdxShClr].percent * (cl.cshifts[IdxShClr].destcolor[1] - g)) >> 8;
            b += (cl.cshifts[IdxShClr].percent * (cl.cshifts[IdxShClr].destcolor[2] - b)) >> 8;
        }

        newpal[0] = gammatable[r];
        newpal[1] = gammatable[g];
        newpal[2] = gammatable[b];
        //-------------------
        newpal += 3;
    }

    VID_ShiftPalette(pal);
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
void CalcGunAngle() {
    static float _oldYaw = 0;
    static float _oldPitch = 0;

    float yaw = r_refdef.viewangles.yaw;
    yaw = angledelta(yaw - r_refdef.viewangles.yaw) * 0.4;
    CLAMP(-10, yaw, 10);
    float move = host_frametime * 20;
    if (yaw > _oldYaw) {
        if ((_oldYaw + move) < yaw)
            yaw = _oldYaw + move;
    }
    else {
        if ((_oldYaw - move) > yaw)
            yaw = _oldYaw - move;
    }

    float pitch = -r_refdef.viewangles.pitch;
    pitch = angledelta(-pitch - r_refdef.viewangles.pitch) * 0.4;
    CLAMP(-10, pitch, 10);
    if (pitch > _oldPitch) {
        if ((_oldPitch + move) < pitch)
            pitch = _oldPitch + move;
    }
    else {
        if ((_oldPitch - move) > pitch)
            pitch = _oldPitch - move;
    }

    _oldYaw = yaw;
    _oldPitch = pitch;

    cl.viewent.angles.yaw = r_refdef.viewangles.yaw + yaw;
    cl.viewent.angles.pitch = -(r_refdef.viewangles.pitch + pitch);

    cl.viewent.angles.roll -= v_idlescale.value * sin(cl.time * v_iroll_cycle.value) * v_iroll_level.value;
    cl.viewent.angles.pitch -= v_idlescale.value * sin(cl.time * v_ipitch_cycle.value) * v_ipitch_level.value;
    cl.viewent.angles.yaw -= v_idlescale.value * sin(cl.time * v_iyaw_cycle.value) * v_iyaw_level.value;
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

    CLAMP(ent->origin.x - 14, r_refdef.vieworg.x, ent->origin.x + 14);
    CLAMP(ent->origin.y - 14, r_refdef.vieworg.y, ent->origin.y + 14);
    CLAMP(ent->origin.z - 22, r_refdef.vieworg.z, ent->origin.z + 30);
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle() {
#if 0
    r_refdef.viewangles.roll += v_idlescale.value * sin(cl.time * v_iroll_cycle.value) * v_iroll_level.value;
    r_refdef.viewangles.pitch += v_idlescale.value * sin(cl.time * v_ipitch_cycle.value) * v_ipitch_level.value;
    r_refdef.viewangles.yaw += v_idlescale.value * sin(cl.time * v_iyaw_cycle.value) * v_iyaw_level.value;
#else
    vec3_t v_i = (vec3_t){
        .pitch = sin(cl.time * v_ipitch_cycle.value) * v_ipitch_level.value,
        .yaw = sin(cl.time * v_iyaw_cycle.value) * v_iyaw_level.value,
        .roll = sin(cl.time * v_iroll_cycle.value) * v_iroll_level.value,
    };
    r_refdef.viewangles = VectorMA(r_refdef.viewangles, v_idlescale.value, v_i);
#endif
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll() {
    float side = V_CalcRoll(cl_entities[cl.viewentity].angles, cl.velocity);
    r_refdef.viewangles.roll += side;

    if (_v_DmgTime > 0.0f) {
        r_refdef.viewangles.roll += _v_DmgTime / v_kicktime.value * _v_DmgRoll;
        r_refdef.viewangles.pitch += _v_DmgTime / v_kicktime.value * _v_DmgPitch;
        _v_DmgTime -= host_frametime;
    }

    if (cl.stats[STAT_HEALTH] <= 0.0f) {
        r_refdef.viewangles.roll = 80.0f; // dead view angle
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

    r_refdef.vieworg = ent->origin;
    r_refdef.viewangles = ent->angles;
    view->model = NULL;

    // allways idle in intermission
    float old = v_idlescale.value;
    v_idlescale.value = 1;
    V_AddIdle();
    v_idlescale.value = old;
}

/*
==================
V_CalcRefdef

==================
*/
void V_CalcRefdef() {
    static float _oldZ = 0;

    V_DriftPitch();
    r_Entity_p ent = &cl_entities[cl.viewentity];   // ent is the player model (visible when out of body)
    r_Entity_p view = &cl.viewent;  // view is the weapon model (only visible from inside body)


    // transform the view offset by the model's matrix to get the offset from model origin for the view
    ent->angles.yaw = cl.viewangles.yaw; // the model should face the view dir
    ent->angles.pitch = -cl.viewangles.pitch; // the model should face the view dir


    float bob = V_CalcBob();
    // refresh position
    r_refdef.vieworg = ent->origin;
    r_refdef.vieworg.z += cl.viewheight + bob;

    // never let it sit exactly on a node line, because a water plane can
    // dissapear when viewed with the eye exactly on it.
    // the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
    r_refdef.vieworg.x += 1.0 / 32;
    r_refdef.vieworg.y += 1.0 / 32;
    r_refdef.vieworg.z += 1.0 / 32;

    r_refdef.viewangles = cl.viewangles;
    V_CalcViewRoll();
    V_AddIdle();

    vec3_t angles = {
        // offsets
       .x = -ent->angles.pitch, /* angles[PITCH] */ // because entity pitches are actually backward
       .y = ent->angles.yaw,    /* angles[YAW] */
       .z = ent->angles.roll    /* angles[ROLL] */
    };

    _bs = GetBasis(angles);

    r_refdef.vieworg =
        VectorMA(VectorMA(VectorMA(r_refdef.vieworg,
            scr_ofsz.value, _bs.up),
            scr_ofsy.value, _bs.right),
            scr_ofsx.value, _bs.forward
        );


    V_BoundOffsets();

    view->angles = cl.viewangles;    // set up gun position

    CalcGunAngle();

    view->origin = ent->origin;
    view->origin.z += cl.viewheight;

    view->origin = VectorMA(view->origin, bob * 0.4f, _bs.forward);
    // view->origin = VectorMA(view->origin, bob * 0.4f, _bs.right);
    // view->origin = VectorMA(view->origin, bob * 0.8f, _bs.up);
    view->origin.z += bob;

    // fudge position around to keep amount of weapon visible
    // roughly equal with different FOV

#if 0
    if (cl.model_precache[cl.stats[STAT_WEAPON]] && strcmp(cl.model_precache[cl.stats[STAT_WEAPON]]->name, "progs/v_shot2.mdl")) {}
#endif
    /**/ if (scr_viewsize.value == 110) view->origin.z += 1;
    else if (scr_viewsize.value == 100) view->origin.z += 2;
    else if (scr_viewsize.value == 90)  view->origin.z += 1;
    else if (scr_viewsize.value == 80)  view->origin.z += 0.5;

    view->model = cl.model_precache[cl.stats[STAT_WEAPON]];
    view->frame = cl.stats[STAT_WEAPONFRAME];
    view->colormap = vid.colormap;

    // set up the refresh position
    r_refdef.viewangles = VectorAdd(r_refdef.viewangles, cl.punchangle);

    // smooth out stair step ups
    if ((cl.onground) &&
        ((ent->origin.z - _oldZ) > 0)) {

        float steptime = cl.time - cl.oldtime;
        if (steptime < 0) {
            //FIXME  I_Error ("steptime < 0");
            steptime = 0;
        }

        _oldZ += steptime * 80;
        if (_oldZ > ent->origin.z)         _oldZ = ent->origin.z;
        if ((ent->origin.z - _oldZ) > 12)  _oldZ = ent->origin.z - 12;
        r_refdef.vieworg.z += _oldZ - ent->origin.z;
        view->origin.z += _oldZ - ent->origin.z;
    }
    else { _oldZ = ent->origin.z; }

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

    if (cl.intermission != IM_NONE) { // intermission / finale rendering
        V_CalcIntermissionRefdef();
    }
    else {
        if (!cl.paused /* && ((sv.maxClients > 1) || (key.dest == key_game)) */)
            V_CalcRefdef();
    }

    R_PushDlights();

    if (lcd_x.value) {
        //
        // render two interleaved views
        //

        vid.rowbytes = TWICE(vid.rowbytes);
        scr.aspect *= 0.5;

        r_refdef.viewangles.yaw -= lcd_yaw.value;
        r_refdef.vieworg = VectorMA(r_refdef.vieworg, -lcd_x.value, _bs.right);
        R_RenderView();
        vid.scr.pBuff += HALF(vid.rowbytes);

        R_PushDlights();

        r_refdef.viewangles.yaw += lcd_yaw.value * 2;

        r_refdef.vieworg = VectorMA(r_refdef.vieworg, 2 * lcd_x.value, _bs.right);
        R_RenderView();
        vid.scr.pBuff -= HALF(vid.rowbytes);

        r_refdef.vrect.height = TWICE(r_refdef.vrect.height);

        vid.rowbytes = HALF(vid.rowbytes);
        scr.aspect *= 2;
    }
    else {
        R_RenderView();
    }

    Draw_crosshair();

}


void Draw_crosshair() {
    if (crosshair.value)
    Draw_Character(
#if GLQUAKE
        scr.vrect.x + HALF(scr.vrect.width),
        scr.vrect.y + HALF(scr.vrect.height),
#else
        scr.vrect.x + HALF(scr.vrect.width) + cl_crossx.value,
        scr.vrect.y + HALF(scr.vrect.height) + cl_crossy.value,
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
    Cvar_SetValue("viewsize", scr_viewsize.value + 10);
    SCR_RequestCalcRefdef();
}


/*
=================
V_SizeDown_f

Keybinding command
=================
*/
void V_SizeDown_f() {
    Cvar_SetValue("viewsize", scr_viewsize.value - 10);
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

    Cvar_RegisterVariable(&lcd_x);
    Cvar_RegisterVariable(&lcd_yaw);

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


    BuildGammaTable(1.0); // no gamma yet
    Cvar_RegisterVariable(&v_gamma);
}


