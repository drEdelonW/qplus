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
    if (beamlength == 0.0f)      return;

#if 0
    vec3_t vec = VectorScale(r_spritedesc.vpn, -beamlength); // vec = r_spritedesc.vpn * (-beamlength);
#else
    vec3_t vec = VectorScale(r_spritedesc.bs.forward, -beamlength); // vec = r_spritedesc.vpn * (-beamlength);
#endif
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
#if 1
int R_ClipSpriteFace(int nump, ClipPlane_p pclipplane) {
    // printf("R_ClipSpriteFace\n");
    float clipdist = pclipplane->dist;
    vec3_t pclipnormal = pclipplane->normal;

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
    int vsize = sizeof(vec5_t) / sizeof(float);
    float dists[MAXWORKINGVERTS + 1];
    for (int i = 0; i < nump; i++, instep += vsize) {
        dists[i] = DotProduct(*(vec3_p)instep, pclipnormal) - clipdist;
    }

    // handle wraparound case
    dists[nump] = dists[0];
    Q_memcpy(instep, in, sizeof(vec5_t));


    // clip the winding
    instep = in;
    int outcount = 0;

    for (int i = 0; i < nump; i++, instep += vsize) {
        if (dists[i] >= 0) {
            Q_memcpy(outstep, instep, sizeof(vec5_t));
            outstep += vsize;
            outcount++;
        }

        if ((dists[i] == 0.0f) || (dists[i + 1] == 0.0f))     continue;
        if ((dists[i] > 0.0f) == (dists[i + 1] > 0.0f))       continue;

        // split it into a new vertex
        float frac = dists[i] / (dists[i] - dists[i + 1]);

        float_p vert2 = instep + vsize;

        for (int v = 0; v < vsize; v++)
            outstep[v] = instep[v] + frac * (vert2[v] - instep[v]);

        outstep += vsize;
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
    int vsize = sizeof(vec5_t) / sizeof(float);
    float dists[MAXWORKINGVERTS + 1];
    for (int i = 0; i < nump; i++, instep += vsize) {
        dists[i] = DotProduct(instep, *pclipnormal) - clipdist;
    }

    // handle wraparound case
    dists[nump] = dists[0];
    Q_memcpy(instep, in, sizeof(vec5_t));


    // clip the winding
    instep = in;
    int outcount = 0;

    for (int i = 0; i < nump; i++, instep += vsize) {
        if (dists[i] >= 0) {
            Q_memcpy(outstep, instep, sizeof(vec5_t));
            outstep += vsize;
            outcount++;
        }

        if ((dists[i] == 0) || (dists[i + 1] == 0))     continue;
        if ((dists[i] > 0) == (dists[i + 1] > 0))       continue;

        // split it into a new vertex
        float frac = dists[i] / (dists[i] - dists[i + 1]);

        float_p vert2 = instep + vsize;    // vec5_t

        for (int v = 0; v < vsize; v++)
            outstep[v] = instep[v] + frac * (vert2[v] - instep[v]);

        outstep += vsize;
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
#if 0
    float dot = DotProduct(r_spritedesc.vpn, modelorg);
#else
    float dot = DotProduct(r_spritedesc.bs.forward, modelorg);
#endif
    // backface cull
    if (dot >= 0)       return;

    // build the sprite poster in worldspace
#if 0
    vec3_t right = VectorScale(r_spritedesc.vright, r_spritedesc.pspriteframe->right);
    vec3_t up = VectorScale(r_spritedesc.vup, r_spritedesc.pspriteframe->up);
    vec3_t left = VectorScale(r_spritedesc.vright, r_spritedesc.pspriteframe->left);
    vec3_t down = VectorScale(r_spritedesc.vup, r_spritedesc.pspriteframe->down);
#else
    vec3_t right = VectorScale(r_spritedesc.bs.right, r_spritedesc.pspriteframe->right);
    vec3_t up = VectorScale(r_spritedesc.bs.up, r_spritedesc.pspriteframe->up);
    vec3_t left = VectorScale(r_spritedesc.bs.right, r_spritedesc.pspriteframe->left);
    vec3_t down = VectorScale(r_spritedesc.bs.up, r_spritedesc.pspriteframe->down);
#endif

    vec5_p pverts = _clip_verts[0];

    pverts[0][0] = r_entorigin.x + up.x + left.x;
    pverts[0][1] = r_entorigin.y + up.y + left.y;
    pverts[0][2] = r_entorigin.z + up.z + left.z;
    pverts[0][3] = 0.0f;
    pverts[0][4] = 0.0f;

    pverts[1][0] = r_entorigin.x + up.x + right.x;
    pverts[1][1] = r_entorigin.y + up.y + right.y;
    pverts[1][2] = r_entorigin.z + up.z + right.z;
    pverts[1][3] = _sprite_width;
    pverts[1][4] = 0.0f;

    pverts[2][0] = r_entorigin.x + down.x + right.x;
    pverts[2][1] = r_entorigin.y + down.y + right.y;
    pverts[2][2] = r_entorigin.z + down.z + right.z;
    pverts[2][3] = _sprite_width;
    pverts[2][4] = _sprite_height;

    pverts[3][0] = r_entorigin.x + down.x + left.x;
    pverts[3][1] = r_entorigin.y + down.y + left.y;
    pverts[3][2] = r_entorigin.z + down.z + left.z;
    pverts[3][3] = 0.0f;
    pverts[3][4] = _sprite_height;

    // clip to the frustum in worldspace
    int nump = 4;
    _clipCurrent = 0;

    for (int i = 0; i < 4; i++) {
        nump = R_ClipSpriteFace(nump, &view_clipplanes[i]);
        if (nump < 3)                   return;
        if (nump >= MAXWORKINGVERTS)    Host_SysError("R_SetupAndDrawSprite: too many points");
    }

    // transform vertices into viewspace and project
    float_p pv = &_clip_verts[_clipCurrent][0][0];
    r_spritedesc.nearzi = -999999.0f;

    EmitPoint_t outverts[MAXWORKINGVERTS + 1];
    for (int i = 0; i < nump; i++) {
        vec3_t local = VectorSubtract(*(vec3_p)pv, r_origin);
        vec3_t transformed = TransformVector(local);

        if (transformed.z < NEAR_CLIP)
            transformed.z = NEAR_CLIP;

        EmitPoint_p pout = &outverts[i];
        pout->zi = 1.0 / transformed.z;
        if (pout->zi > r_spritedesc.nearzi)
            r_spritedesc.nearzi = pout->zi;

        pout->s = pv[3];
        pout->t = pv[4];
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
            .x = -modelorg.x,
            .y = -modelorg.y,
            .z = -modelorg.z
        };
        VectorNormalize(&tvec);
        float dot = tvec.z; // same as DotProduct (tvec, r_spritedesc.vup) because r_spritedesc.vup is 0, 0, 1
        if ((dot > 0.999848f) ||
            (dot < -0.999848f)) // cos(1 degree) = 0.999848
            return;

#if 0
        r_spritedesc.vup.x = 0;
        r_spritedesc.vup.y = 0;
        r_spritedesc.vup.z = 1;

        r_spritedesc.vright.x = tvec.y;
        // r_spritedesc.vright = CrossProduct(r_spritedesc.vup, -modelorg);
        r_spritedesc.vright.y = -tvec.x;
        r_spritedesc.vright.z = 0;
        VectorNormalize(&r_spritedesc.vright);

        r_spritedesc.vpn.x = -r_spritedesc.vright.y;
        r_spritedesc.vpn.y = r_spritedesc.vright.x;
        r_spritedesc.vpn.z = 0;
        // r_spritedesc.vpn = CrossProduct(r_spritedesc.vright, r_spritedesc.vup);
#else
        r_spritedesc.bs.up.x = 0;
        r_spritedesc.bs.up.y = 0;
        r_spritedesc.bs.up.z = 1;

        r_spritedesc.bs.right.x = tvec.y;
        // r_spritedesc.vright = CrossProduct(r_spritedesc.vup, -modelorg);
        r_spritedesc.bs.right.y = -tvec.x;
        r_spritedesc.bs.right.z = 0;
        VectorNormalize(&r_spritedesc.bs.right);

        r_spritedesc.bs.forward.x = -r_spritedesc.bs.right.y;
        r_spritedesc.bs.forward.y = r_spritedesc.bs.right.x;
        r_spritedesc.bs.forward.z = 0;
        // r_spritedesc.vpn = CrossProduct(r_spritedesc.vright, r_spritedesc.vup);
#endif
    }
    else if (psprite->type == SPR_VP_PARALLEL) {
        // generate the sprite's axes, completely parallel to the viewplane. There
        // are no problem situations, because the sprite is always in the same
        // position relative to the viewer
#if 0
        r_spritedesc.vup = vup;
        r_spritedesc.vright = vright;
        r_spritedesc.vpn = vpn;
#else
        r_spritedesc.bs = BS;
#endif
    }
    else if (psprite->type == SPR_VP_PARALLEL_UPRIGHT) {
        // generate the sprite's axes, with vup straight up in worldspace, and
        // r_spritedesc.vright parallel to the viewplane.
        // This will not work if the view direction is very close to straight up or
        // down, because the cross product will be between two nearly parallel
        // vectors and starts to approach an undefined state, so we don't draw if
        // the two vectors are less than 1 degree apart
#if 0
        float dot = vpn.z; // same as DotProduct (vpn, r_spritedesc.vup) because
#else
        float dot = BS.forward.z; // same as DotProduct (vpn, r_spritedesc.vup) because
#endif
        //  r_spritedesc.vup is 0, 0, 1
        if ((dot > 0.999848f) ||
            (dot < -0.999848f)) // cos(1 degree) = 0.999848
            return;

#if 0
        r_spritedesc.vup = (vec3_t){
#else
        r_spritedesc.bs.up = (vec3_t){
#endif
             .x = 0.0f,
             .y = 0.0f,
             .z = 0.0f
        };

        //  r_spritedesc.vright = CrossProduct(r_spritedesc.vup, vpn)
#if 0
        r_spritedesc.vright = (vec3_t){
             .x = vpn.y,
             .y = -vpn.x,
             .z = 0.0f
        };
        VectorNormalize(&r_spritedesc.vright);

        r_spritedesc.vpn = (vec3_t){
             .x = -r_spritedesc.vright.y,
             .y = r_spritedesc.vright.x,
             .z = 0.0f
        };
#else
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
#endif
        // r_spritedesc.vpn = CrossProduct(r_spritedesc.vright, r_spritedesc.vup)
    }
    else if (psprite->type == SPR_ORIENTED) {
        // generate the sprite's axes, according to the sprite's world orientation
#if 0
        AngleVectors(currententity->angles, &r_spritedesc.vpn, &r_spritedesc.vright, &r_spritedesc.vup);
#else
        r_spritedesc.bs = GetBasis(currententity->angles);
#endif
    }
    else if (psprite->type == SPR_VP_PARALLEL_ORIENTED) {
        // generate the sprite's axes, parallel to the viewplane, but rotated in
        // that plane around the center according to the sprite entity's roll
        // angle. So vpn stays the same, but vright and vup rotate
        float angle = currententity->angles.roll * (M_PI * 2.0f / 360.0f);
        float sr = sin(angle);
        float cr = cos(angle);

#if 0
        r_spritedesc.vpn = vpn;
        r_spritedesc.vright = VectorMA(VectorScale(vright, cr), sr, vup);
        r_spritedesc.vup = VectorMA(VectorScale(vright, -sr), cr, vup);
#else
        r_spritedesc.bs = (Basis_t){
            .forward = BS.forward,
            .right = VectorMA(
                VectorScale(BS.right, cr),
                 sr, BS.up
                ),
            .up = VectorMA(
                VectorScale(BS.right, -sr),
                cr, BS.up
                )
        };
#endif
    }
    else {
        Host_SysError("R_DrawSprite: Bad sprite type %d", psprite->type);
    }

    R_RotateSprite(psprite->beamlength);

    R_SetupAndDrawSprite();
}

