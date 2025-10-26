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
// r_alias.c: routines for setting up to draw alias models

#include "r_local.h"
#include "d_local.h" // FIXME: shouldn't be needed (is needed for patch
                        // right now, but that should move)

#define LIGHT_MIN (5)  // lowest light value we'll allow, to avoid the
                            //  need for inner-loop light clamping

mTriangle_p     ptriangles;
AffineTriDesc_t r_affinetridesc;
TypeLess_ptr    acolormap; // FIXME: should go away
TriVertx_p      r_apverts;

// TODO: these probably will go away with optimized rasterization
Mdl_p   pmdl;
vec3_t  r_plightvec;
int     r_ambientlight;
float   r_shadelight;
AliasHdr_p  paliashdr;
FinalVert_p pfinalverts;
AuxVert_p   pauxverts;

static float    _ziscale;
static Model_p  _pmodel;
static vec3_t   _aliasForward, _aliasRight, _aliasUp;
static mAliasSkinDesc_p _pSkinDesc;

int r_amodels_drawn;
int a_skinwidth;
int r_anumverts;

float aliastransform[3][4];

typedef struct {
    int index0;
    int index1;
} aEdge_t;

static aEdge_t _aEdges[12] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 5}, {1, 4}, {2, 7}, {3, 6}
};

#define NUMVERTEXNORMALS 162

float r_avertexnormals[NUMVERTEXNORMALS][3] = {
#   include "anorms.h"
};

void R_AliasTransformAndProjectFinalVerts(FinalVert_p fv, stvert_p pstverts);
void R_AliasSetUpTransform(int trivial_accept);
void R_AliasTransformVector(vec3_t in, vec3_t out);
void R_AliasTransformFinalVert(FinalVert_p fv, AuxVert_p av, TriVertx_p pverts, stvert_p pstverts);
void R_AliasProjectFinalVert(FinalVert_p fv, AuxVert_p av);


/*
================
R_AliasCheckBBox
================
*/
qboolean R_AliasCheckBBox() {
    // expand, rotate, and translate points into worldspace

    currententity->trivial_accept = 0;
    _pmodel = currententity->model;
    AliasHdr_p pahdr = Mod_Extradata(_pmodel);
    pmdl = (Mdl_p)((uint8_p)pahdr + pahdr->model);

    R_AliasSetUpTransform(0);

    // construct the base bounding box for this frame
    int frame = currententity->frame;
    // TODO: don't repeat this check when drawing?
    if ((frame >= pmdl->numframes) ||
        (frame < 0)) {
        Con_DPrintf("No such frame %d %s\n", frame,
            _pmodel->name);
        frame = 0;
    }

    mAliasFrameDesc_p pframedesc = &pahdr->frames[frame];

    float basepts[8][3];
    // x worldspace coordinates
    basepts[0][0] =
        basepts[1][0] =
        basepts[2][0] =
        basepts[3][0] = (float)pframedesc->bboxmin.v[0];
    basepts[4][0] =
        basepts[5][0] =
        basepts[6][0] =
        basepts[7][0] = (float)pframedesc->bboxmax.v[0];

    // y worldspace coordinates
    basepts[0][1] =
        basepts[3][1] =
        basepts[5][1] =
        basepts[6][1] = (float)pframedesc->bboxmin.v[1];
    basepts[1][1] =
        basepts[2][1] =
        basepts[4][1] =
        basepts[7][1] = (float)pframedesc->bboxmax.v[1];

    // z worldspace coordinates
    basepts[0][2] =
        basepts[1][2] =
        basepts[4][2] =
        basepts[5][2] = (float)pframedesc->bboxmin.v[2];
    basepts[2][2] =
        basepts[3][2] =
        basepts[6][2] =
        basepts[7][2] = (float)pframedesc->bboxmax.v[2];

    qboolean zclipped = false;
    qboolean zfullyclipped = true;

    int minz = 9999;
    AuxVert_t   viewaux[16];
    FinalVert_t viewpts[16];
    for (int i = 0; i < 8; i++) {
        R_AliasTransformVector(&basepts[i][0], &viewaux[i].fv[0]);

        if (viewaux[i].fv[2] < ALIAS_Z_CLIP_PLANE) {
            // we must clip points that are closer than the near clip plane
            viewpts[i].flags = ALIAS_Z_CLIP;
            zclipped = true;
        }
        else {
            if (viewaux[i].fv[2] < minz)
                minz = viewaux[i].fv[2];
            viewpts[i].flags = 0;
            zfullyclipped = false;
        }
    }


    if (zfullyclipped) {
        return false; // everything was near-z-clipped
    }

    int numv = 8;

    if (zclipped) {
        // organize points by edges, use edges to get new points (possible trivial
        // reject)
        for (int i = 0; i < 12; i++) {
            // edge endpoints
            FinalVert_p pv0 = &viewpts[_aEdges[i].index0];
            FinalVert_p pv1 = &viewpts[_aEdges[i].index1];
            AuxVert_p pa0 = &viewaux[_aEdges[i].index0];
            AuxVert_p pa1 = &viewaux[_aEdges[i].index1];

            // if one end is clipped and the other isn't, make a new point
            if (pv0->flags ^ pv1->flags) {
                float frac =
                    (ALIAS_Z_CLIP_PLANE - pa0->fv[2]) /
                    (pa1->fv[2] - pa0->fv[2]);
                viewaux[numv].fv[0] =
                    pa0->fv[0] +
                    (pa1->fv[0] - pa0->fv[0]) * frac;
                viewaux[numv].fv[1] =
                    pa0->fv[1] +
                    (pa1->fv[1] - pa0->fv[1]) * frac;
                viewaux[numv].fv[2] = ALIAS_Z_CLIP_PLANE;
                viewpts[numv].flags = 0;
                numv++;
            }
        }
    }

    // project the vertices that remain after clipping
    uint32_t anyclip = 0;
    uint32_t allclip = ALIAS_XY_CLIP_MASK;

    // TODO: probably should do this loop in ASM, especially if we use floats
    for (int i = 0; i < numv; i++) {
        // we don't need to bother with vertices that were z-clipped
        if (viewpts[i].flags & ALIAS_Z_CLIP)
            continue;

        float zi = 1.0 / viewaux[i].fv[2];

        // FIXME: do with chop mode in ASM, or convert to float
        float v0 = (viewaux[i].fv[0] * xscale * zi) + xcenter;
        float v1 = (viewaux[i].fv[1] * yscale * zi) + ycenter;

        int flags = 0;

        if (v0 < r_refdef.fvrectx)      flags |= ALIAS_LEFT_CLIP;
        if (v1 < r_refdef.fvrecty)      flags |= ALIAS_TOP_CLIP;
        if (v0 > r_refdef.fvrectright)  flags |= ALIAS_RIGHT_CLIP;
        if (v1 > r_refdef.fvrectbottom) flags |= ALIAS_BOTTOM_CLIP;

        anyclip |= flags;
        allclip &= flags;
    }

    if (allclip)
        return false; // trivial reject off one side

    currententity->trivial_accept = !anyclip & !zclipped;

    if ((currententity->trivial_accept) &&
        (minz > (r_aliastransition + (pmdl->size * r_resfudge)))) {
        currententity->trivial_accept |= 2;
    }

    return true;
}


/*
================
R_AliasTransformVector
================
*/
void R_AliasTransformVector(vec3_t in, vec3_t out) {
    for (int i = 0; i < 3; i++)
        out[i] = DotProduct(in, aliastransform[i]) + aliastransform[i][3];
}


/*
================
R_AliasPreparePoints

General clipped case
================
*/
void R_AliasPreparePoints() {
    stvert_p pstverts = (stvert_p)((uint8_p)paliashdr + paliashdr->stverts);
    r_anumverts = pmdl->numverts;
    FinalVert_p fv = pfinalverts;
    AuxVert_p av = pauxverts;

    for (int i = 0; i < r_anumverts; i++, fv++, av++, r_apverts++, pstverts++) {
        R_AliasTransformFinalVert(fv, av, r_apverts, pstverts);
        if (av->fv[2] < ALIAS_Z_CLIP_PLANE)
            fv->flags |= ALIAS_Z_CLIP;
        else {
            R_AliasProjectFinalVert(fv, av);

            if (fv->v[0] < r_refdef.aliasvrect.x)       fv->flags |= ALIAS_LEFT_CLIP;
            if (fv->v[1] < r_refdef.aliasvrect.y)       fv->flags |= ALIAS_TOP_CLIP;
            if (fv->v[0] > r_refdef.aliasvrectright)    fv->flags |= ALIAS_RIGHT_CLIP;
            if (fv->v[1] > r_refdef.aliasvrectbottom)   fv->flags |= ALIAS_BOTTOM_CLIP;
        }
    }

    //
    // clip and draw all triangles
    //
    r_affinetridesc.numtriangles = 1;

    mTriangle_p ptri = (mTriangle_p)((uint8_p)paliashdr + paliashdr->triangles);
    for (int i = 0; i < pmdl->numtris; i++, ptri++) {
        FinalVert_p pfv[3] = {
            &pfinalverts[ptri->vertindex[0]],
            &pfinalverts[ptri->vertindex[1]],
            &pfinalverts[ptri->vertindex[2]]
        };

        if (pfv[0]->flags &
            pfv[1]->flags &
            pfv[2]->flags &
            (ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP))
            continue;  // completely clipped

        if (
            !(
                (
                    pfv[0]->flags |
                    pfv[1]->flags |
                    pfv[2]->flags) &
                (
                    ALIAS_XY_CLIP_MASK |
                    ALIAS_Z_CLIP)
                )
            ) { // totally unclipped
            r_affinetridesc.pfinalverts = pfinalverts;
            r_affinetridesc.ptriangles = ptri;
            D_PolysetDraw();
        }
        else { // partially clipped
            R_AliasClipTriangle(ptri);
        }
    }
}


/*
================
R_AliasSetUpTransform
================
*/
void R_AliasSetUpTransform(int trivial_accept) {
    static float tmatrix[3][4];
    static float viewmatrix[3][4];

    // TODO: should really be stored with the entity instead of being reconstructed
    // TODO: should use a look-up table
    // TODO: could cache lazily, stored in the entity

    vec3_t angles = {
        /* angles[PITCH] = */ -currententity->angles[PITCH],
        /* angles[YAW] = */ currententity->angles[YAW],
        /* angles[ROLL] = */ currententity->angles[ROLL]
    };
    AngleVectors(angles, _aliasForward, _aliasRight, _aliasUp);

    tmatrix[0][0] = pmdl->scale[0];
    tmatrix[1][1] = pmdl->scale[1];
    tmatrix[2][2] = pmdl->scale[2];

    tmatrix[0][3] = pmdl->scale_origin[0];
    tmatrix[1][3] = pmdl->scale_origin[1];
    tmatrix[2][3] = pmdl->scale_origin[2];

    // TODO: can do this with simple matrix rearrangement
    float rotationmatrix[3][4], t2matrix[3][4];
    for (int i = 0; i < VECT_DIM; i++) {
        t2matrix[i][0] = _aliasForward[i];
        t2matrix[i][1] = -_aliasRight[i];
        t2matrix[i][2] = _aliasUp[i];
    }

    t2matrix[0][3] = -modelorg[0];
    t2matrix[1][3] = -modelorg[1];
    t2matrix[2][3] = -modelorg[2];

    // FIXME: can do more efficiently than full concatenation
    R_ConcatTransforms(t2matrix, tmatrix, rotationmatrix);

    // TODO: should be global, set when vright, etc., set
    VectorCopy(vright, viewmatrix[0]);
    VectorCopy(vup, viewmatrix[1]);
    VectorInverse(viewmatrix[1]);
    VectorCopy(vpn, viewmatrix[2]);

    // viewmatrix[0][3] = 0;
    // viewmatrix[1][3] = 0;
    // viewmatrix[2][3] = 0;

    R_ConcatTransforms(viewmatrix, rotationmatrix, aliastransform);

    // do the scaling up of x and y to screen coordinates as part of the transform
    // for the unclipped case (it would mess up clipping in the clipped case).
    // Also scale down z, so 1/z is scaled 31 bits for free, and scale down x and y
    // correspondingly so the projected x and y come out right
    // FIXME: make this work for clipped case too?
    if (trivial_accept) {
        for (int i = 0; i < 4; i++) {
            aliastransform[0][i] *= aliasxscale * (1.0 / ((float)0x8000 * 0x10000));
            aliastransform[1][i] *= aliasyscale * (1.0 / ((float)0x8000 * 0x10000));
            aliastransform[2][i] *= (1.0 / ((float)0x8000 * 0x10000));
        }
    }
}


/*
================
R_AliasTransformFinalVert
================
*/
void R_AliasTransformFinalVert(FinalVert_p fv, AuxVert_p av, TriVertx_p pverts, stvert_p pstverts) {
    av->fv[0] = DotProduct(pverts->v, aliastransform[0]) + aliastransform[0][3];
    av->fv[1] = DotProduct(pverts->v, aliastransform[1]) + aliastransform[1][3];
    av->fv[2] = DotProduct(pverts->v, aliastransform[2]) + aliastransform[2][3];

    fv->v[2] = pstverts->s;
    fv->v[3] = pstverts->t;

    fv->flags = pstverts->onseam;

    // lighting
    float_p plightnormal = r_avertexnormals[pverts->lightnormalindex];
    float lightcos = DotProduct(plightnormal, r_plightvec);
    int temp = r_ambientlight;

    if (lightcos < 0) {
        temp += (int)(r_shadelight * lightcos);

        // clamp; because we limited the minimum ambient and shading light, we
        // don't have to clamp low light, just bright
        if (temp < 0)
            temp = 0;
    }

    fv->v[4] = temp;
}


#if !id386

/*
================
R_AliasTransformAndProjectFinalVerts
================
*/
void R_AliasTransformAndProjectFinalVerts(FinalVert_p fv, stvert_p pstverts) {
    TriVertx_p pverts = r_apverts;

    for (int i = 0; i < r_anumverts; i++, fv++, pverts++, pstverts++) {
        // transform and project
        float zi = 1.0 / (DotProduct(pverts->v, aliastransform[2]) + aliastransform[2][3]);

        // x, y, and z are scaled down by 1/2**31 in the transform, so 1/z is
        // scaled up by 1/2**31, and the scaling cancels out for x and y in the
        // projection
        fv->v[5] = zi;

        fv->v[0] = ((DotProduct(pverts->v, aliastransform[0]) + aliastransform[0][3]) * zi) + aliasxcenter;
        fv->v[1] = ((DotProduct(pverts->v, aliastransform[1]) + aliastransform[1][3]) * zi) + aliasycenter;

        fv->v[2] = pstverts->s;
        fv->v[3] = pstverts->t;
        fv->flags = pstverts->onseam;

        // lighting
        float_p plightnormal = r_avertexnormals[pverts->lightnormalindex];
        float lightcos = DotProduct(plightnormal, r_plightvec);
        int temp = r_ambientlight;

        if (lightcos < 0) {
            temp += (int)(r_shadelight * lightcos);

            // clamp; because we limited the minimum ambient and shading light, we
            // don't have to clamp low light, just bright
            if (temp < 0)
                temp = 0;
        }

        fv->v[4] = temp;
    }
}

#endif


/*
================
R_AliasProjectFinalVert
================
*/
void R_AliasProjectFinalVert(FinalVert_p fv, AuxVert_p av) {
    // project points
    float zi = 1.0 / av->fv[2];

    fv->v[5] = zi * _ziscale;

    fv->v[0] = (av->fv[0] * aliasxscale * zi) + aliasxcenter;
    fv->v[1] = (av->fv[1] * aliasyscale * zi) + aliasycenter;
}


/*
================
R_AliasPrepareUnclippedPoints
================
*/
void R_AliasPrepareUnclippedPoints() {
    stvert_p pstverts = (stvert_p)((uint8_p)paliashdr + paliashdr->stverts);
    r_anumverts = pmdl->numverts;
    // FIXME: just use pfinalverts directly?
    FinalVert_p fv = pfinalverts;

    R_AliasTransformAndProjectFinalVerts(fv, pstverts);

    if (r_affinetridesc.drawtype)
        D_PolysetDrawFinalVerts(fv, r_anumverts);

    r_affinetridesc.pfinalverts = pfinalverts;
    r_affinetridesc.ptriangles = (mTriangle_p)
        ((uint8_p)paliashdr + paliashdr->triangles);
    r_affinetridesc.numtriangles = pmdl->numtris;

    D_PolysetDraw();
}

/*
===============
R_AliasSetupSkin
===============
*/
void R_AliasSetupSkin() {
    int skinnum = currententity->skinnum;
    if ((skinnum >= pmdl->numskins) ||
        (skinnum < 0)) {
        Con_DPrintf("R_AliasSetupSkin: no such skin # %d\n", skinnum);
        skinnum = 0;
    }

    _pSkinDesc = ((mAliasSkinDesc_p)
        ((uint8_p)paliashdr + paliashdr->skindesc)) +
        skinnum;
    a_skinwidth = pmdl->skinwidth;

    if (_pSkinDesc->type == ALIAS_SKIN_GROUP) {
        mAliasSkinGroup_p paliasskingroup = (mAliasSkinGroup_p)((uint8_p)paliashdr + _pSkinDesc->skin);
        float_p pskinintervals = (float_p)((uint8_p)paliashdr + paliasskingroup->intervals);
        int numskins = paliasskingroup->numskins;
        float fullskininterval = pskinintervals[numskins - 1];
        float skintime = cl.time + currententity->syncbase;

        // when loading in Mod_LoadAliasSkinGroup, we guaranteed all interval
        // values are positive, so we don't have to worry about division by 0
        float skintargettime =
            skintime -
            ((int)(skintime / fullskininterval)) * fullskininterval;
        int i = 0;
        for (; i < (numskins - 1); i++) {
            if (pskinintervals[i] > skintargettime)
                break;
        }

        _pSkinDesc = &paliasskingroup->skindescs[i];
    }

    r_affinetridesc.pskindesc = _pSkinDesc;
    r_affinetridesc.pskin = (TypeLess_ptr)((uint8_p)paliashdr + _pSkinDesc->skin);
    r_affinetridesc.skinwidth = a_skinwidth;
    r_affinetridesc.seamfixupX16 = (a_skinwidth >> 1) << 16;
    r_affinetridesc.skinheight = pmdl->skinheight;
}

/*
================
R_AliasSetupLighting
================
*/
void R_AliasSetupLighting(aLight_p plighting) {

    // guarantee that no vertex will ever be lit below LIGHT_MIN, so we don't have
    // to clamp off the bottom
    r_ambientlight = plighting->ambientlight;
    if (r_ambientlight < LIGHT_MIN)
        r_ambientlight = LIGHT_MIN;

    r_ambientlight = (255 - r_ambientlight) << VID_CBITS;
    if (r_ambientlight < LIGHT_MIN)
        r_ambientlight = LIGHT_MIN;

    r_shadelight = plighting->shadelight;
    if (r_shadelight < 0)
        r_shadelight = 0;

    r_shadelight *= VID_GRADES;

    // rotate the lighting vector into the model's frame of reference
    r_plightvec[0] = DotProduct(plighting->plightvec, _aliasForward);
    r_plightvec[1] = -DotProduct(plighting->plightvec, _aliasRight);
    r_plightvec[2] = DotProduct(plighting->plightvec, _aliasUp);
}

/*
=================
R_AliasSetupFrame

set r_apverts
=================
*/
void R_AliasSetupFrame() {
    int frame = currententity->frame;
    if ((frame >= pmdl->numframes) ||
        (frame < 0)) {
        Con_DPrintf("R_AliasSetupFrame: no such frame %d\n", frame);
        frame = 0;
    }

    if (paliashdr->frames[frame].type == ALIAS_SINGLE) {
        r_apverts = (TriVertx_p)((uint8_p)paliashdr + paliashdr->frames[frame].frame);
        return;
    }

    mAliasGroup_p paliasgroup = (mAliasGroup_p)((uint8_p)paliashdr + paliashdr->frames[frame].frame);
    float_p pintervals = (float_p)((uint8_p)paliashdr + paliasgroup->intervals);
    int numframes = paliasgroup->numframes;
    float fullinterval = pintervals[numframes - 1];
    float time = cl.time + currententity->syncbase;

    //
    // when loading in Mod_LoadAliasGroup, we guaranteed all interval values
    // are positive, so we don't have to worry about division by 0
    //
    float targettime = time - ((int)(time / fullinterval)) * fullinterval;

    int i = 0;
    for (; i < (numframes - 1); i++) {
        if (pintervals[i] > targettime)
            break;
    }

    r_apverts = (TriVertx_p)((uint8_p)paliashdr + paliasgroup->frames[i].frame);
}


/*
================
R_AliasDrawModel
================
*/
void R_AliasDrawModel(aLight_p plighting) {
    FinalVert_t finalverts[
        MAXALIASVERTS +
            ((CACHE_SIZE - 1) /
                sizeof(FinalVert_t)) +
            1];

    r_amodels_drawn++;

    // cache align
    pfinalverts = (FinalVert_p)(((uintptr_t)&finalverts[0] + CACHE_SIZE - 1) & ~(uintptr_t)(CACHE_SIZE - 1));
    AuxVert_t auxverts[MAXALIASVERTS];
    pauxverts = &auxverts[0];

    paliashdr = (AliasHdr_p)Mod_Extradata(currententity->model);
    pmdl = (Mdl_p)((uint8_p)paliashdr + paliashdr->model);

    R_AliasSetupSkin();
    R_AliasSetUpTransform(currententity->trivial_accept);
    R_AliasSetupLighting(plighting);
    R_AliasSetupFrame();

    if (!currententity->colormap)
        Sys_Error("R_AliasDrawModel: !currententity->colormap");

    r_affinetridesc.drawtype =
        (currententity->trivial_accept == 3) &&
        r_recursiveaffinetriangles;

    if (r_affinetridesc.drawtype) {
        D_PolysetUpdateTables();  // FIXME: precalc...
    }
#if id386
    else {
        D_Aff8Patch(currententity->colormap);
    }
#endif

    acolormap = currententity->colormap;

    if (currententity != &cl.viewent)
        _ziscale = (float)0x8000 * (float)0x10000;
    else
        _ziscale = (float)0x8000 * (float)0x10000 * 3.0;

    if (currententity->trivial_accept)
        R_AliasPrepareUnclippedPoints();
    else
        R_AliasPreparePoints();
}

