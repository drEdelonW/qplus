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
// r_sprite.c

#include "q_tools.h"
#include "r_local.h"
#include "host.h"
#include "console.h"

static bool     _clipCurrent = false;
static vec5_t   _clip_verts[2][MAXWORKINGVERTS];
static int      _sprite_width, _sprite_height;

SpriteDesc_t    r_spritedesc;


/*
================
R_RotateSprite
================
*/
void R_RotateSprite(float beamlength) {
    if (beamlength == 0.0f)      return;

    vec3_t vec = VectorScale(r_spritedesc.bs.forward, -beamlength); // vec = r_spritedesc.bs.forward * (-beamlength);
    r_entorigin = VectorAdd(r_entorigin, vec);   // r_entorigin += vec;
    modelorg = VectorSubtract(modelorg, vec);    // modelorg -= vec;
}


/*
=============
R_ClipSpriteFace

Clips the winding at _clip_verts[_clipCurrent] and changes _clipCurrent
Throws out the back side
==============
*/
int R_ClipSpriteFace(int nump, ClipPlane_p pclipplane) {
    // calc dists
    _clipCurrent = !_clipCurrent;
    vec5_p in = &_clip_verts[!_clipCurrent][0];
    vec5_p outstep = &_clip_verts[_clipCurrent][0];

    vec5_p instep = in;
    float dists[MAXWORKINGVERTS + 1];
    int i = 0;
    for (; i < nump; i++) {
        dists[i] = DotProduct(instep[i].vx, pclipplane->normal) - pclipplane->dist;
    }

    // handle wraparound case
    dists[nump] = dists[0];
    instep[i] = *in;

    // clip the winding
    instep = in;
    int outcount = 0;
    for (int i = 0; i < nump; i++) {
        if (dists[i] >= 0) {
            *outstep = instep[i];
            outstep++;
            outcount++;
        }

        if ((dists[i] == 0.0f) || (dists[i + 1] == 0.0f))     continue;
        if ((dists[i] > 0.0f) == (dists[i + 1] > 0.0f))       continue;

        // split it into a new vertex
        float frac = dists[i] / (dists[i] - dists[i + 1]);

        for (int v = 0; v < sizeof(vec5_t); v++)
            outstep->arr[v] = (
                instep[i].arr[v] +
                frac * (
                    instep[i + 1].arr[v] -
                    instep[i].arr[v])
                );

        outstep++;
        outcount++;
    }

    return outcount;
}

/*
================
R_SetupAndDrawSprite
================
*/
void R_SetupAndDrawSprite() {

    float dot = DotProduct(r_spritedesc.bs.forward, modelorg);
    // backface cull
    if (dot >= 0)       return;

    // build the sprite poster in worldspace
    vec3_t right = VectorScale(r_spritedesc.bs.right, r_spritedesc.pspriteframe->right);
    vec3_t up = VectorScale(r_spritedesc.bs.up, r_spritedesc.pspriteframe->up);
    vec3_t left = VectorScale(r_spritedesc.bs.right, r_spritedesc.pspriteframe->left);
    vec3_t down = VectorScale(r_spritedesc.bs.up, r_spritedesc.pspriteframe->down);

    vec3_t EntUp = VectorAdd(r_entorigin, up);
    vec5_p pverts = _clip_verts[0];
    pverts[0] = (vec5_t){
        .vx = VectorAdd(EntUp, left),
        .vt = {.s = 0.0f,           .t = 0.0f}
    };
    pverts[1] = (vec5_t){
        .vx = VectorAdd(EntUp, right),
        .vt = {.s = _sprite_width,  .t = 0.0f}
    };
    vec3_t EntDown = VectorAdd(r_entorigin, down);
    pverts[2] = (vec5_t){
        .vx = VectorAdd(EntDown, right),
        .vt = {.s = _sprite_width,  .t = _sprite_height}
    };
    pverts[3] = (vec5_t){
        .vx = VectorAdd(EntDown, left),
        .vt = {.s = 0.0f,           .t = _sprite_height}
    };

    // clip to the frustum in worldspace
    int nump = 4;
    _clipCurrent = false;

    for (int i = 0; i < 4; i++) {
        nump = R_ClipSpriteFace(nump, &view_clipplanes[i]);
        if (nump < 3)                   return;
        if (nump >= MAXWORKINGVERTS)    Host_SysError("R_SetupAndDrawSprite: too many points");
    }

    // transform vertices into viewspace and project
    vec5_p pv = &_clip_verts[_clipCurrent][0];
    r_spritedesc.nearzi = -999999.0f;

    EmitPoint_t outverts[MAXWORKINGVERTS + 1];
    for (int i = 0; i < nump; i++) {
        vec3_t local = VectorSubtract(pv->vx, r_origin);
        vec3_t transformed = TransformVector(local);
        ClampLessThen(&transformed.z, NEAR_CLIP);

        EmitPoint_p pout = &outverts[i];
        pout->zi = 1.0 / transformed.z;
        ClampLessThen(&r_spritedesc.nearzi, pout->zi);

        pout->s = pv->s;
        pout->t = pv->t;
        pout->u = xcenter + xscale * pout->zi * transformed.x;
        pout->v = ycenter - yscale * pout->zi * transformed.y;
        pv += sizeof(vec5_t) / sizeof(*pv);
    }

    // draw it
    r_spritedesc.nump = nump;
    r_spritedesc.pverts = outverts;
    D_DrawSprite();
}


/*
================
R_GetSpriteframe
================
*/
mSpriteFrame_p R_GetSpriteframe(mSprite_p psprite) { // TODO: seems like OpenGL function as is
    int frame = currententity->frame;
    if ((frame >= psprite->numframes) ||
        (frame < 0)
        ) {
        Con_Printf("R_DrawSprite: no such frame %d\n", frame);
        frame = 0;
    }

    mSpriteFrame_p pspriteframe;
    if (psprite->frames[frame].type == SPR_SINGLE) {
        pspriteframe = psprite->frames[frame].frameptr;
    }
    else {
        mSpriteGroup_p pspritegroup = (mSpriteGroup_p)psprite->frames[frame].frameptr;
        LegDt_p pintervals = pspritegroup->intervals;   // TODO: replace by time interval specific type
        int numframes = pspritegroup->numframes;
        float fullinterval = pintervals[numframes - 1];

        float time = GetClSimTime() + currententity->syncbase;

        // when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
        // are positive, so we don't have to worry about division by 0
        float targettime = time - ((int)(time / fullinterval)) * fullinterval;

        int i = 0;
        for (; i < (numframes - 1); i++) {
            if (pintervals[i] > targettime)
                break;
        }

        pspriteframe = pspritegroup->frames[i];
    }

    return pspriteframe;
}


/*
================
R_DrawSprite
================
*/
void R_DrawSprite() {
    mSprite_p psprite = currententity->model->cache.data;
    r_spritedesc.pspriteframe = R_GetSpriteframe(psprite);

    _sprite_width = r_spritedesc.pspriteframe->width;
    _sprite_height = r_spritedesc.pspriteframe->height;

    // TODO: make this caller-selectable
    if (psprite->type == SPR_FACING_UPRIGHT) {
        // generate the sprite's axes, with vup straight up in worldspace, and
        // r_spritedesc.vright perpendicular to modelorg.
        // This will not work if the view direction is very close to straight up or
        // down, because the cross product will be between two nearly parallel
        // vectors and starts to approach an undefined state, so we don't draw if
        // the two vectors are less than 1 degree apart
        vec3_t tvec = { .x = -modelorg.x, .y = -modelorg.y, .z = -modelorg.z };
        VectorNormalize(&tvec);
        float dot = tvec.z; // same as DotProduct (tvec, r_spritedesc.vup) because r_spritedesc.vup is 0, 0, 1
        if ((dot > 0.999848f) ||
            (dot < -0.999848f)) // cos(1 degree) = 0.999848
            return;

        r_spritedesc.bs.up = (vec3_t){
            .x = 0.0f,
            .y = 0.0f,
            .z = 1.0f
        };
        // r_spritedesc.bs.right = CrossProduct(r_spritedesc.bs.up, -modelorg);

        r_spritedesc.bs.right = (vec3_t){
            .x = tvec.y,
            .y = -tvec.x,
            .z = 0.0f
        };
        VectorNormalize(&r_spritedesc.bs.right);

        r_spritedesc.bs.forward = (vec3_t){
            .x = -r_spritedesc.bs.right.y,
            .y = r_spritedesc.bs.right.x,
            .z = 0.0f
        };
        // r_spritedesc.bs.forward = CrossProduct(r_spritedesc.bs.right, r_spritedesc.bs.up);
    }
    else if (psprite->type == SPR_VP_PARALLEL) {
        // generate the sprite's axes, completely parallel to the viewplane.
        // There are no problem situations, because the sprite is always in the same position relative to the viewer
        r_spritedesc.bs = BS;
    }
    else if (psprite->type == SPR_VP_PARALLEL_UPRIGHT) {
        // generate the sprite's axes, with vup straight up in worldspace, and
        // r_spritedesc.vright parallel to the viewplane.
        // This will not work if the view direction is very close to straight up or
        // down, because the cross product will be between two nearly parallel
        // vectors and starts to approach an undefined state, so we don't draw if
        // the two vectors are less than 1 degree apart
        float dot = BS.forward.z; // same as DotProduct (BS.forward, r_spritedesc.vup) because
        //  r_spritedesc.bs.up is 0, 0, 1
        if ((dot > 0.999848f) ||
            (dot < -0.999848f)) // cos(1 degree) = 0.999848
            return;

        r_spritedesc.bs.up = Scalar2Vector(0.0f);

        //  r_spritedesc.vright = CrossProduct(r_spritedesc.bs.up, BS.forward)
        r_spritedesc.bs.right = (vec3_t){
            .x = BS.forward.y,
            .y = -BS.forward.x,
            .z = 0.0f
        };
        VectorNormalize(&r_spritedesc.bs.right);

        r_spritedesc.bs.forward = (vec3_t){
            .x = -r_spritedesc.bs.right.y,
            .y = r_spritedesc.bs.right.x,
            .z = 0.0f
        };
        // r_spritedesc.bs.forward = CrossProduct(r_spritedesc.bs.right, r_spritedesc.bs.up)
    }
    else if (psprite->type == SPR_ORIENTED) {
        // generate the sprite's axes, according to the sprite's world orientation
        r_spritedesc.bs = GetBasis(currententity->pose.aim);
    }
    else if (psprite->type == SPR_VP_PARALLEL_ORIENTED) {
        // generate the sprite's axes, parallel to the viewplane, but rotated in
        // that plane around the center according to the sprite entity's roll
        // angle. So 'forward' stays the same, but 'right' and 'up' rotate
        float angle = currententity->pose.aim.roll * (M_PI * 2.0f / 360.0f);
        float sr = sinf(angle);
        float cr = cosf(angle);

        r_spritedesc.bs = (Basis_t){
            .forward = BS.forward,
            .right = VectorMA(
                VectorScale(BS.right, cr),
                sr, BS.up),
            .up = VectorMA(
                VectorScale(BS.right, -sr),
                cr, BS.up)
        };
    }
    else { Host_SysError("R_DrawSprite: Bad sprite type %d", psprite->type); }

    R_RotateSprite(psprite->beamlength);
    R_SetupAndDrawSprite();
}

