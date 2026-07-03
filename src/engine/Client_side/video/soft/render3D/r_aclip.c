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
// r_aclip.c: clip routines for drawing Alias models directly to the screen

#include "r_local.h"
#include "d_local.h"
#include "q_tools.h"

static FinalVert_t  _fv[2][8];
static AuxVert_t  _av[8];

void R_AliasProjectFinalVert(FinalVert_p _fv, AuxVert_p _av);
void R_Alias_clip_top(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out);
void R_Alias_clip_bottom(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out);
void R_Alias_clip_left(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out);
void R_Alias_clip_right(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out);


/*
================
R_Alias_clip_z

pfv0 is the unclipped vertex, pfv1 is the z-clipped vertex
================
*/
void R_Alias_clip_z(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out) {
    AuxVert_t avout;
    AuxVert_p pav0 = &_av[pfv0 - &_fv[0][0]];
    AuxVert_p pav1 = &_av[pfv1 - &_fv[0][0]];

    if (pfv0->vAttr.y >= pfv1->vAttr.y) {
        float scale =
            (ALIAS_Z_CLIP_PLANE - pav0->fv.z) /
            (pav1->fv.z - pav0->fv.z);

        // Linear interpolation: avout = pav0 + scale * (pav1 - pav0)
        avout.fv = VectorMA(pav0->fv, scale, VectorSubtract(pav1->fv, pav0->fv));
        avout.fv.z = ALIAS_Z_CLIP_PLANE;

        out->vAttr.s = pfv0->vAttr.s + (pfv1->vAttr.s - pfv0->vAttr.s) * scale;
        out->vAttr.t = pfv0->vAttr.t + (pfv1->vAttr.t - pfv0->vAttr.t) * scale;
        out->vAttr.light = pfv0->vAttr.light + (pfv1->vAttr.light - pfv0->vAttr.light) * scale;
    }
    else {
        float scale =
            (ALIAS_Z_CLIP_PLANE - pav1->fv.z) /
            (pav0->fv.z - pav1->fv.z);
        // Linear interpolation: avout = pav1 + scale * (pav0 - pav1)
        avout.fv = VectorMA(pav1->fv, scale, VectorSubtract(pav0->fv, pav1->fv));
        avout.fv.z = ALIAS_Z_CLIP_PLANE;

        out->vAttr.s = pfv1->vAttr.s + (pfv0->vAttr.s - pfv1->vAttr.s) * scale;
        out->vAttr.t = pfv1->vAttr.t + (pfv0->vAttr.t - pfv1->vAttr.t) * scale;
        out->vAttr.light = pfv1->vAttr.light + (pfv0->vAttr.light - pfv1->vAttr.light) * scale;
    }

    R_AliasProjectFinalVert(out, &avout);

    if (out->vAttr.x < r_refdef.aliasvrect.x)      out->flags |= ALIAS_LEFT_CLIP;
    if (out->vAttr.y < r_refdef.aliasvrect.y)      out->flags |= ALIAS_TOP_CLIP;
    if (out->vAttr.x > r_refdef.aliasvrectright)   out->flags |= ALIAS_RIGHT_CLIP;
    if (out->vAttr.y > r_refdef.aliasvrectbottom)  out->flags |= ALIAS_BOTTOM_CLIP;
}


#if !id386

void R_Alias_clip_left(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out) {
    if (pfv0->vAttr.y >= pfv1->vAttr.y) {
        float scale = (float)(
            r_refdef.aliasvrect.x - pfv0->vAttr.x) /
            (pfv1->vAttr.x - pfv0->vAttr.x);
        for (int i = 0; i < 6; i++)
            out->vAttr.q16[i] = pfv0->vAttr.q16[i] + (pfv1->vAttr.q16[i] - pfv0->vAttr.q16[i]) * scale + 0.5;
    }
    else {
        float scale = (float)(
            r_refdef.aliasvrect.x - pfv1->vAttr.x) /
            (pfv0->vAttr.x - pfv1->vAttr.x);
        for (int i = 0; i < 6; i++)
            out->vAttr.q16[i] = pfv1->vAttr.q16[i] + (pfv0->vAttr.q16[i] - pfv1->vAttr.q16[i]) * scale + 0.5;
    }
}


void R_Alias_clip_right(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out) {
    if (pfv0->vAttr.y >= pfv1->vAttr.y) {
        float scale = (float)(
            r_refdef.aliasvrectright - pfv0->vAttr.x) /
            (pfv1->vAttr.x - pfv0->vAttr.x);
        for (int i = 0; i < 6; i++)
            out->vAttr.q16[i] = pfv0->vAttr.q16[i] + (pfv1->vAttr.q16[i] - pfv0->vAttr.q16[i]) * scale + 0.5;
    }
    else {
        float scale = (float)(
            r_refdef.aliasvrectright - pfv1->vAttr.x) /
            (pfv0->vAttr.x - pfv1->vAttr.x);
        for (int i = 0; i < 6; i++)
            out->vAttr.q16[i] = pfv1->vAttr.q16[i] + (pfv0->vAttr.q16[i] - pfv1->vAttr.q16[i]) * scale + 0.5;
    }
}


void R_Alias_clip_top(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out) {
    if (pfv0->vAttr.y >= pfv1->vAttr.y) {
        float scale = (float)(
            r_refdef.aliasvrect.y - pfv0->vAttr.y) /
            (pfv1->vAttr.y - pfv0->vAttr.y);
        for (int i = 0; i < 6; i++)
            out->vAttr.q16[i] = pfv0->vAttr.q16[i] + (pfv1->vAttr.q16[i] - pfv0->vAttr.q16[i]) * scale + 0.5;
    }
    else {
        float scale =
            (float)(r_refdef.aliasvrect.y - pfv1->vAttr.y) /
            (pfv0->vAttr.y - pfv1->vAttr.y);
        for (int i = 0; i < 6; i++)
            out->vAttr.q16[i] = pfv1->vAttr.q16[i] + (pfv0->vAttr.q16[i] - pfv1->vAttr.q16[i]) * scale + 0.5;
    }
}


void R_Alias_clip_bottom(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out) {
    if (pfv0->vAttr.y >= pfv1->vAttr.y) {
        float scale = (float)(
            r_refdef.aliasvrectbottom - pfv0->vAttr.y) /
            (pfv1->vAttr.y - pfv0->vAttr.y);
        for (int i = 0; i < 6; i++)
            out->vAttr.q16[i] = pfv0->vAttr.q16[i] + (pfv1->vAttr.q16[i] - pfv0->vAttr.q16[i]) * scale + 0.5;
    }
    else {
        float scale = (float)(
            r_refdef.aliasvrectbottom - pfv1->vAttr.y) /
            (pfv0->vAttr.y - pfv1->vAttr.y);
        for (int i = 0; i < 6; i++)
            out->vAttr.q16[i] = pfv1->vAttr.q16[i] + (pfv0->vAttr.q16[i] - pfv1->vAttr.q16[i]) * scale + 0.5;
    }
}

#endif

typedef void (*alias_clip_fn_t)(FinalVert_p pfv0, FinalVert_p pfv1, FinalVert_p out);

int R_AliasClip(FinalVert_p in, FinalVert_p out, int flag, int count, alias_clip_fn_t clip_fn) {
    int j = count - 1;
    int k = 0;
    for (int i = 0; i < count; j = i, i++) {
        int oldflags = in[j].flags & flag;
        int flags = in[i].flags & flag;

        if (flags && oldflags)
            continue;
        if (oldflags ^ flags) {
            clip_fn(&in[j], &in[i], &out[k]);
            out[k].flags = 0;
            if (out[k].vAttr.x < r_refdef.aliasvrect.x)    out[k].flags |= ALIAS_LEFT_CLIP;
            if (out[k].vAttr.y < r_refdef.aliasvrect.y)    out[k].flags |= ALIAS_TOP_CLIP;
            if (out[k].vAttr.x > r_refdef.aliasvrectright) out[k].flags |= ALIAS_RIGHT_CLIP;
            if (out[k].vAttr.y > r_refdef.aliasvrectbottom) out[k].flags |= ALIAS_BOTTOM_CLIP;
            k++;
        }
        if (!flags) {
            out[k] = in[i];
            k++;
        }
    }

    return k;
}


/*
================
R_AliasClipTriangle
================
*/
void R_AliasClipTriangle(mTriangle_p ptri) {
    // copy vertexes and fix seam texture coordinates
    if (ptri->facesfront) {
        for (int i = 0; i < 3; i++)
            _fv[0][i] = pfinalverts[ptri->vertindex[i]];
    }
    else {
        for (int i = 0; i < 3; i++) {
            _fv[0][i] = pfinalverts[ptri->vertindex[i]];

            if (!(ptri->facesfront) && (_fv[0][i].flags & ALIAS_ONSEAM))
                _fv[0][i].vAttr.s += r_affinetridesc.seamfixupX16;
        }
    }

    // clip
    AliasClipFlags_f clipflags =
        _fv[0][0].flags |
        _fv[0][1].flags |
        _fv[0][2].flags;

    int k;
    int pingpong;
    if (clipflags & ALIAS_Z_CLIP) {
        for (int i = 0; i < 3; i++)
            _av[i] = pauxverts[ptri->vertindex[i]];

        k = R_AliasClip(_fv[0], _fv[1], ALIAS_Z_CLIP, 3, R_Alias_clip_z);
        if (k == 0)
            return;

        pingpong = 1;
        clipflags =
            _fv[1][0].flags |
            _fv[1][1].flags |
            _fv[1][2].flags;
    }
    else {
        pingpong = 0;
        k = 3;
    }

    if (clipflags & ALIAS_LEFT_CLIP) {
        k = R_AliasClip(_fv[pingpong], _fv[pingpong ^ 1],
            ALIAS_LEFT_CLIP, k, R_Alias_clip_left);
        if (k == 0)
            return;

        pingpong ^= 1;
    }

    if (clipflags & ALIAS_RIGHT_CLIP) {
        k = R_AliasClip(_fv[pingpong], _fv[pingpong ^ 1],
            ALIAS_RIGHT_CLIP, k, R_Alias_clip_right);
        if (k == 0)
            return;

        pingpong ^= 1;
    }

    if (clipflags & ALIAS_BOTTOM_CLIP) {
        k = R_AliasClip(_fv[pingpong], _fv[pingpong ^ 1],
            ALIAS_BOTTOM_CLIP, k, R_Alias_clip_bottom);
        if (k == 0)
            return;

        pingpong ^= 1;
    }

    if (clipflags & ALIAS_TOP_CLIP) {
        k = R_AliasClip(_fv[pingpong], _fv[pingpong ^ 1],
            ALIAS_TOP_CLIP, k, R_Alias_clip_top);
        if (k == 0)
            return;

        pingpong ^= 1;
    }

    for (int i = 0; i < k; i++) {
        CLAMP(r_refdef.aliasvrect.x, &_fv[pingpong][i].vAttr.x, r_refdef.aliasvrectright);
        CLAMP(r_refdef.aliasvrect.y, &_fv[pingpong][i].vAttr.y, r_refdef.aliasvrectbottom);
        _fv[pingpong][i].flags = 0;
    }

    // draw triangles
    mTriangle_t mtri = {
        .facesfront = ptri->facesfront
    };
    r_affinetridesc.ptriangles = &mtri;
    r_affinetridesc.pfinalverts = _fv[pingpong];

    // FIXME: do all at once as trifan?
    mtri.vertindex[0] = 0;
    for (int i = 1; i < k - 1; i++) {
        mtri.vertindex[1] = i;
        mtri.vertindex[2] = i + 1;
        D_PolysetDraw();
    }
}

