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

#include "quakedef.h"
#include "r_local.h"

static int      _clipCurrent = 0;
static vec5_t   _clip_verts[2][MAXWORKINGVERTS];
static int      _sprite_width, _sprite_height;

SpriteDesc_t    r_spritedesc;


/*
================
R_RotateSprite
================
*/
void R_RotateSprite(float beamlength) {
    if (beamlength == 0.0)      return;

    vec3_t vec; VectorScale(r_spritedesc.vpn, -beamlength, vec); // vec = r_spritedesc.vpn * (-beamlength);
    VectorAdd(r_entorigin, vec, r_entorigin);   // r_entorigin += vec;
    VectorSubtract(modelorg, vec, modelorg);    // modelorg -= vec;
}


/*
=============
R_ClipSpriteFace

Clips the winding at _clip_verts[_clipCurrent] and changes _clipCurrent
Throws out the back side
==============
*/
#if 1
int R_ClipSpriteFace(int nump, ClipPlane_p pclipplane) {
    printf("R_ClipSpriteFace\n");
    float clipdist = pclipplane->dist;
    float_p pclipnormal = pclipplane->normal;

    // calc dists
    float_p in;
    float_p outstep;
    if (_clipCurrent) {
        in = _clip_verts[1][0];
        outstep = _clip_verts[0][0];
        _clipCurrent = 0;
    }
    else {
        in = _clip_verts[0][0];
        outstep = _clip_verts[1][0];
        _clipCurrent = 1;
    }

    float_p instep = in;
    float dists[MAXWORKINGVERTS + 1];
    for (int i = 0; i < nump; i++, instep += sizeof(vec5_t) / sizeof(float)) {
        dists[i] = DotProduct(instep, pclipnormal) - clipdist;
    }

    // handle wraparound case
    dists[nump] = dists[0];
    Q_memcpy(instep, in, sizeof(vec5_t));


    // clip the winding
    instep = in;
    int outcount = 0;

    for (int i = 0; i < nump; i++, instep += sizeof(vec5_t) / sizeof(float)) {
        if (dists[i] >= 0) {
            Q_memcpy(outstep, instep, sizeof(vec5_t));
            outstep += sizeof(vec5_t) / sizeof(float);
            outcount++;
        }

        if ((dists[i] == 0) || (dists[i + 1] == 0))     continue;
        if ((dists[i] > 0) == (dists[i + 1] > 0))       continue;

        // split it into a new vertex
        float frac = dists[i] / (dists[i] - dists[i + 1]);

        float_p vert2 = instep + sizeof(vec5_t) / sizeof(float);

        outstep[0] = instep[0] + frac * (vert2[0] - instep[0]);
        outstep[1] = instep[1] + frac * (vert2[1] - instep[1]);
        outstep[2] = instep[2] + frac * (vert2[2] - instep[2]);
        outstep[3] = instep[3] + frac * (vert2[3] - instep[3]);
        outstep[4] = instep[4] + frac * (vert2[4] - instep[4]);

        outstep += sizeof(vec5_t) / sizeof(float);
        outcount++;
    }

    return outcount;
}
#else
int R_ClipSpriteFace(int nump, ClipPlane_p pclipplane) {
    printf("R_ClipSpriteFace\n");
    float clipdist = pclipplane->dist;
    vec3_p pclipnormal = &pclipplane->normal;

    // calc dists
    float_p in;         // vec5_t
    float_p outstep;    // vec5_t
#if 1
    if (_clipCurrent) {
        in = _clip_verts[1][0];
        outstep = _clip_verts[0][0];
        _clipCurrent = 0;
    }
    else {
        in = _clip_verts[0][0];
        outstep = _clip_verts[1][0];
        _clipCurrent = 1;
    }
#else
    _clipCurrent = !_clipCurrent;
    in = _clip_verts[!_clipCurrent][0];
    outstep = _clip_verts[_clipCurrent][0];
#endif

    float_p instep = in;    // vec5_t
    float dists[MAXWORKINGVERTS + 1];
    for (int i = 0; i < nump; i++, instep += sizeof(vec5_t) / sizeof(float)) {
        dists[i] = DotProduct(instep, *pclipnormal) - clipdist;
    }

    // handle wraparound case
    dists[nump] = dists[0];
    Q_memcpy(instep, in, sizeof(vec5_t));


    // clip the winding
    instep = in;
    int outcount = 0;

    for (int i = 0; i < nump; i++, instep += sizeof(vec5_t) / sizeof(float)) {
        if (dists[i] >= 0) {
            Q_memcpy(outstep, instep, sizeof(vec5_t));
            outstep += sizeof(vec5_t) / sizeof(float);
            outcount++;
        }

        if ((dists[i] == 0) || (dists[i + 1] == 0))     continue;
        if ((dists[i] > 0) == (dists[i + 1] > 0))       continue;

        // split it into a new vertex
        float frac = dists[i] / (dists[i] - dists[i + 1]);

        float_p vert2 = instep + sizeof(vec5_t) / sizeof(float);    // vec5_t

        outstep[0] = instep[0] + frac * (vert2[0] - instep[0]);
        outstep[1] = instep[1] + frac * (vert2[1] - instep[1]);
        outstep[2] = instep[2] + frac * (vert2[2] - instep[2]);
        outstep[3] = instep[3] + frac * (vert2[3] - instep[3]);
        outstep[4] = instep[4] + frac * (vert2[4] - instep[4]);

        outstep += sizeof(vec5_t) / sizeof(float);
        outcount++;
    }

    return outcount;
}
#endif

/*
================
R_SetupAndDrawSprite
================
*/
void R_SetupAndDrawSprite() {
    float dot = DotProduct(r_spritedesc.vpn, modelorg);
    // backface cull
    if (dot >= 0)       return;

    // build the sprite poster in worldspace
    vec3_t right;   VectorScale(r_spritedesc.vright, r_spritedesc.pspriteframe->right, right);
    vec3_t up;      VectorScale(r_spritedesc.vup, r_spritedesc.pspriteframe->up, up);
    vec3_t left;    VectorScale(r_spritedesc.vright, r_spritedesc.pspriteframe->left, left);
    vec3_t down;    VectorScale(r_spritedesc.vup, r_spritedesc.pspriteframe->down, down);

    vec5_p pverts = _clip_verts[0];

    pverts[0][0] = r_entorigin[0] + up[0] + left[0];
    pverts[0][1] = r_entorigin[1] + up[1] + left[1];
    pverts[0][2] = r_entorigin[2] + up[2] + left[2];
    pverts[0][3] = 0;
    pverts[0][4] = 0;

    pverts[1][0] = r_entorigin[0] + up[0] + right[0];
    pverts[1][1] = r_entorigin[1] + up[1] + right[1];
    pverts[1][2] = r_entorigin[2] + up[2] + right[2];
    pverts[1][3] = _sprite_width;
    pverts[1][4] = 0;

    pverts[2][0] = r_entorigin[0] + down[0] + right[0];
    pverts[2][1] = r_entorigin[1] + down[1] + right[1];
    pverts[2][2] = r_entorigin[2] + down[2] + right[2];
    pverts[2][3] = _sprite_width;
    pverts[2][4] = _sprite_height;

    pverts[3][0] = r_entorigin[0] + down[0] + left[0];
    pverts[3][1] = r_entorigin[1] + down[1] + left[1];
    pverts[3][2] = r_entorigin[2] + down[2] + left[2];
    pverts[3][3] = 0;
    pverts[3][4] = _sprite_height;

    // clip to the frustum in worldspace
    int nump = 4;
    _clipCurrent = 0;

    for (int i = 0; i < 4; i++) {
        nump = R_ClipSpriteFace(nump, &view_clipplanes[i]);
        if (nump < 3)                   return;
        if (nump >= MAXWORKINGVERTS)    Sys_Error("R_SetupAndDrawSprite: too many points");
    }

    // transform vertices into viewspace and project
    float_p pv = &_clip_verts[_clipCurrent][0][0];
    r_spritedesc.nearzi = -999999;

    EmitPoint_t outverts[MAXWORKINGVERTS + 1];
    for (int i = 0; i < nump; i++) {
        vec3_t local;   VectorSubtract(pv, r_origin, local);
        vec3_t transformed; TransformVector(local, transformed);

        if (transformed[2] < NEAR_CLIP)
            transformed[2] = NEAR_CLIP;

        EmitPoint_p pout = &outverts[i];
        pout->zi = 1.0 / transformed[2];
        if (pout->zi > r_spritedesc.nearzi)
            r_spritedesc.nearzi = pout->zi;

        pout->s = pv[3];
        pout->t = pv[4];
        pout->u = xcenter + xscale * pout->zi * transformed[0];
        pout->v = ycenter - yscale * pout->zi * transformed[1];
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
mSpriteFrame_p R_GetSpriteframe(mSprite_p psprite) {
    int frame = currententity->frame;
    if ((frame >= psprite->numframes) || (frame < 0)) {
        Con_Printf("R_DrawSprite: no such frame %d\n", frame);
        frame = 0;
    }

    mSpriteFrame_p pspriteframe;
    if (psprite->frames[frame].type == SPR_SINGLE) {
        pspriteframe = psprite->frames[frame].frameptr;
    }
    else {
        mSpriteGroup_p pspritegroup = (mSpriteGroup_p)psprite->frames[frame].frameptr;
        float_p pintervals = pspritegroup->intervals;
        int numframes = pspritegroup->numframes;
        float fullinterval = pintervals[numframes - 1];

        float time = cl.time + currententity->syncbase;

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
        vec3_t tvec = {
            -modelorg[0],
            -modelorg[1],
            -modelorg[2]
        };
        VectorNormalize(tvec);
        float dot = tvec[2]; // same as DotProduct (tvec, r_spritedesc.vup) because
        //  r_spritedesc.vup is 0, 0, 1
        if ((dot > 0.999848) ||
            (dot < -0.999848)) // cos(1 degree) = 0.999848
            return;
        r_spritedesc.vup[0] = 0;
        r_spritedesc.vup[1] = 0;
        r_spritedesc.vup[2] = 1;
        r_spritedesc.vright[0] = tvec[1];
        // CrossProduct(r_spritedesc.vup, -modelorg,
        r_spritedesc.vright[1] = -tvec[0];
        //              r_spritedesc.vright)
        r_spritedesc.vright[2] = 0;
        VectorNormalize(r_spritedesc.vright);
        r_spritedesc.vpn[0] = -r_spritedesc.vright[1];
        r_spritedesc.vpn[1] = r_spritedesc.vright[0];
        r_spritedesc.vpn[2] = 0;
        // CrossProduct (r_spritedesc.vright, r_spritedesc.vup,
        //  r_spritedesc.vpn)
    }
    else if (psprite->type == SPR_VP_PARALLEL) {
        // generate the sprite's axes, completely parallel to the viewplane. There
        // are no problem situations, because the sprite is always in the same
        // position relative to the viewer
        for (int i = 0; i < VECT_DIM; i++) {
            r_spritedesc.vup[i] = vup[i];
            r_spritedesc.vright[i] = vright[i];
            r_spritedesc.vpn[i] = vpn[i];
        }
    }
    else if (psprite->type == SPR_VP_PARALLEL_UPRIGHT) {
        // generate the sprite's axes, with vup straight up in worldspace, and
        // r_spritedesc.vright parallel to the viewplane.
        // This will not work if the view direction is very close to straight up or
        // down, because the cross product will be between two nearly parallel
        // vectors and starts to approach an undefined state, so we don't draw if
        // the two vectors are less than 1 degree apart
        float dot = vpn[2]; // same as DotProduct (vpn, r_spritedesc.vup) because
        //  r_spritedesc.vup is 0, 0, 1
        if ((dot > 0.999848) ||
            (dot < -0.999848)) // cos(1 degree) = 0.999848
            return;

        r_spritedesc.vup[0] = 0;
        r_spritedesc.vup[1] = 0;
        r_spritedesc.vup[2] = 1;

        r_spritedesc.vright[0] = vpn[1];
        // CrossProduct (r_spritedesc.vup, vpn,
        r_spritedesc.vright[1] = -vpn[0]; //  r_spritedesc.vright)
        r_spritedesc.vright[2] = 0;
        VectorNormalize(r_spritedesc.vright);

        r_spritedesc.vpn[0] = -r_spritedesc.vright[1];
        r_spritedesc.vpn[1] = r_spritedesc.vright[0];
        r_spritedesc.vpn[2] = 0;
        // CrossProduct (r_spritedesc.vright, r_spritedesc.vup,
        //  r_spritedesc.vpn)
    }
    else if (psprite->type == SPR_ORIENTED) {
        // generate the sprite's axes, according to the sprite's world orientation
        AngleVectors(currententity->angles, r_spritedesc.vpn,
            r_spritedesc.vright, r_spritedesc.vup);
    }
    else if (psprite->type == SPR_VP_PARALLEL_ORIENTED) {
        // generate the sprite's axes, parallel to the viewplane, but rotated in
        // that plane around the center according to the sprite entity's roll
        // angle. So vpn stays the same, but vright and vup rotate
        float angle = currententity->angles[ROLL] * (M_PI * 2 / 360);
        float sr = sin(angle);
        float cr = cos(angle);

        for (int i = 0; i < VECT_DIM; i++) {
            r_spritedesc.vpn[i] = vpn[i];
            r_spritedesc.vright[i] = vright[i] * cr + vup[i] * sr;
            r_spritedesc.vup[i] = vright[i] * -sr + vup[i] * cr;
        }
    }
    else {
        Sys_Error("R_DrawSprite: Bad sprite type %d", psprite->type);
    }

    R_RotateSprite(psprite->beamlength);

    R_SetupAndDrawSprite();
}

