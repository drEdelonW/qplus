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

// r_draw.c
#include "r_local.h"
#include "d_local.h" // FIXME: shouldn't need to include this
#include "Surface.h"

#define MAXLEFTCLIPEDGES  (100)

// !!! if these are changed, they must be changed in asm_draw.h too !!!
#define FULLY_CLIPPED_CACHED    (0x80000000)
#define FRAMECOUNT_MASK         (0x7FFFFFFF)

static uint32_t _cacheOffset;
int             c_faceclip;     // number of faces clipped - debug data
PolyDesc_t      r_polydesc;

ClipPlane_t view_clipplanes[4];

static mEdge_p _r_pedge;

static bool _r_leftclipped, _r_rightclipped;
static bool _makeLeftEdge, _makeRightEdge;
static bool _r_nearzionly;

int sintable[SIN_BUFFER_SIZE];
int intsintable[SIN_BUFFER_SIZE];

static mVertex_t _r_leftenter, _r_leftexit;
static mVertex_t _r_rightenter, _r_rightexit;

typedef struct {
    float u;
    float v;
    float lzi;
    int  ceilv;
} evert_t;

static evert_t _r;

static int      _r_emitted;
static float    _r_nearzi;
static bool     _rLastVertValid;

#if !id386

/*
================
R_EmitEdge
================
*/
void R_EmitEdge(mVertex_p pv0, mVertex_p pv1) {
    evert_t em;
    if (_rLastVertValid) {
        em = _r;
    }
    else {
        vec3_t world = pv0->position;

        // transform and project
        vec3_t local = VectorSubtract(world, modelorg);
        vec3_t transformed = TransformVector(local);

        ClampLessThen(&transformed.z, NEAR_CLIP);

        em.lzi = 1.0 / transformed.z;

        // FIXME: build x/yscale into transform?
        {
            float scale = xscale * em.lzi;
            em.u = (xcenter + scale * transformed.x);
            ClampInRange(r_refdef.fvrectx_adj, &em.u, r_refdef.fvrectright_adj);
        }

        {
            float scale = yscale * em.lzi;
            em.v = (ycenter - scale * transformed.y);
            ClampInRange(r_refdef.fvrecty_adj, &em.v, r_refdef.fvrectbottom_adj);
        }
        em.ceilv = (int)ceil(em.v);
    }

    vec3_t world = pv1->position;

    // transform and project
    vec3_t local = VectorSubtract(world, modelorg);
    vec3_t transformed = TransformVector(local);

    ClampLessThen(&transformed.z, NEAR_CLIP);

    _r.lzi = 1.0 / transformed.z;

    {
        float scale = xscale * _r.lzi;
        _r.u = (xcenter + scale * transformed.x);
        ClampInRange(r_refdef.fvrectx_adj, &_r.u, r_refdef.fvrectright_adj);
    }

    {
        float scale = yscale * _r.lzi;
        _r.v = (ycenter - scale * transformed.y);
        ClampInRange(r_refdef.fvrecty_adj, &_r.v, r_refdef.fvrectbottom_adj);
    }

    ClampLessThen(&em.lzi, _r.lzi);
    ClampLessThen(&_r_nearzi, em.lzi);     // for mipmap finding

    // for right edges, all we want is the effect on 1/z
    if (_r_nearzionly)
        return;

    _r_emitted = 1;
    _r.ceilv = (int)ceil(_r.v);


    // create the edge
    if (em.ceilv == _r.ceilv) {
        // we cache unclipped horizontal edges as fully clipped
        if (_cacheOffset != 0x7FFFFFFF) {
            _cacheOffset =
                FULLY_CLIPPED_CACHED |
                (r_framecount & FRAMECOUNT_MASK);
        }

        return;  // horizontal edge
    }

    int side = em.ceilv > _r.ceilv;
    Edge_p edge = edge_p++;
    edge->owner = _r_pedge;
    edge->nearzi = em.lzi;

    float u, u_step;
    int v, v2;
    if (side == 0) {    // trailing edge (go from p1 to p2)
        v = em.ceilv;
        v2 = _r.ceilv - 1;

        edge->surfs[0] = pSurface - pSurfaces;
        edge->surfs[1] = 0;

        u_step = ((_r.u - em.u) / (_r.v - em.v));
        u = em.u + ((float)v - em.v) * u_step;
    }
    else {  // leading edge (go from p2 to p1)
        v2 = em.ceilv - 1;
        v = _r.ceilv;

        edge->surfs[0] = 0;
        edge->surfs[1] = pSurface - pSurfaces;

        u_step = ((em.u - _r.u) / (em.v - _r.v));
        u = _r.u + ((float)v - _r.v) * u_step;
    }

    edge->u_step = u_step * 0x100000;
    edge->u = u * 0x100000 + 0xFFFFF;

    // we need to do this to avoid stepping off the edges if a very nearly
    // horizontal edge is less than epsilon above a scan, and numeric error causes
    // it to incorrectly extend to the scan, and the extension of the line goes off
    // the edge of the screen
    // FIXME: is this actually needed?
    ClampInRange(r_refdef.vrect_x_adj_shift20, &edge->u, r_refdef.vrectright_adj_shift20);

    //
    // sort the edge in normally
    //
    int u_check = edge->u;
    if (edge->surfs[0])
        u_check++; // sort trailers after leaders

    if (!newedges[v] ||
        (newedges[v]->u >= u_check)
        ) {
        edge->next = newedges[v];
        newedges[v] = edge;
    }
    else {
        Edge_p pcheck = newedges[v];
        while (pcheck->next &&
            (pcheck->next->u < u_check)
            )   pcheck = pcheck->next;
        edge->next = pcheck->next;
        pcheck->next = edge;
    }

    edge->nextremove = removeedges[v2];
    removeedges[v2] = edge;
}


/*
================
R_ClipEdge
================
*/
void R_ClipEdge(mVertex_p pv0, mVertex_p pv1, ClipPlane_p clip) {
    if (clip) {
        do {
            float d0 = DotProduct(pv0->position, clip->normal) - clip->dist;
            float d1 = DotProduct(pv1->position, clip->normal) - clip->dist;

            if (d0 >= 0) {      // point 0 is unclipped
                if (d1 >= 0) {  // both points are unclipped
                    continue;
                }
                // only point 1 is clipped
                _cacheOffset = 0x7FFFFFFF;  // we don't cache clipped edges

                float f = d0 / (d0 - d1);
                mVertex_t clipvert;
                clipvert.position = VectorMA(pv0->position, f, VectorSubtract(pv1->position, pv0->position));

                if (clip->leftedge) {
                    _r_leftclipped = true;
                    _r_leftexit = clipvert;
                }
                else if (clip->rightedge) {
                    _r_rightclipped = true;
                    _r_rightexit = clipvert;
                }

                R_ClipEdge(pv0, &clipvert, clip->next);
                return;
            }
            else {              // point 0 is clipped
                if (d1 < 0) {   // both points are clipped
                    // we do cache fully clipped edges
                    if (!_r_leftclipped)
                        _cacheOffset =
                        FULLY_CLIPPED_CACHED |
                        (r_framecount & FRAMECOUNT_MASK);
                    return;
                }

                _rLastVertValid = false;        // only point 0 is clipped
                _cacheOffset = 0x7FFFFFFF;      // we don't cache partially clipped edges

                float f = d0 / (d0 - d1);
                mVertex_t clipvert;
                clipvert.position = VectorMA(pv0->position, f, VectorSubtract(pv1->position, pv0->position));
                if (clip->leftedge) {
                    _r_leftclipped = true;
                    _r_leftenter = clipvert;
                }
                else if (clip->rightedge) {
                    _r_rightclipped = true;
                    _r_rightenter = clipvert;
                }

                R_ClipEdge(&clipvert, pv1, clip->next);
                return;
            }
        } while ((clip = clip->next) != NULL);
    }

    R_EmitEdge(pv0, pv1);   // add the edge
}

#endif // !id386


/*
================
R_EmitCachedEdge
================
*/
void R_EmitCachedEdge() {
    Edge_p pedge_t = (Edge_p)((uintptr_t)r_edges + (uintptr_t)_r_pedge->cachededgeoffset);

    if (!pedge_t->surfs[0])     pedge_t->surfs[0] = pSurface - pSurfaces;
    else                        pedge_t->surfs[1] = pSurface - pSurfaces;

    ClampLessThen(&_r_nearzi, pedge_t->nearzi); // for mipmap finding
    _r_emitted = 1;
}


/*
================
R_RenderFace
================
*/
void R_RenderFace(mSurface_p fa, AliasClipFlags_f clipflags) {
    static mEdge_t _tEdge;

    // skip out if no more surfs
    if ((pSurface) >= pSurf_max) {
        r_outofsurfaces++;
        return;
    }

    // ditto if not enough edges left, or switch to auxedges if possible
    if ((edge_p + fa->numedges + 4) >= edge_max) {
        r_outofedges += fa->numedges;
        return;
    }

    c_faceclip++;

    // set up clip planes
    ClipPlane_p pclip = NULL;

    AliasClipFlags_f mask = ALIAS_BOTTOM_CLIP;
    for (int i = 3; i >= 0; i--, mask >>= 1) {
        if (clipflags & mask) {
            view_clipplanes[i].next = pclip;
            pclip = &view_clipplanes[i];
        }
    }

    // push the edges through
    _r_emitted = 0;
    _r_nearzi = 0;
    _r_nearzionly = false;
    _makeLeftEdge = _makeRightEdge = false;
    mEdge_p pedges = currententity->model->edges;
    _rLastVertValid = false;

    for (int i = 0; i < fa->numedges; i++) {
        int lindex = currententity->model->surfedges[fa->firstedge + i];

        if (lindex > 0) {
            _r_pedge = &pedges[lindex];

            // if the edge is cached, we can just reuse the edge
            if (!inSubModel) {
                if ((_r_pedge->cachededgeoffset & FULLY_CLIPPED_CACHED) &&
                    ((_r_pedge->cachededgeoffset & FRAMECOUNT_MASK) == r_framecount)
                    ) {
                    _rLastVertValid = false;
                    continue;
                }
                else {
                    if ((((uintptr_t)edge_p - (uintptr_t)r_edges) > _r_pedge->cachededgeoffset) &&
                        (((Edge_p)((uintptr_t)r_edges + _r_pedge->cachededgeoffset))->owner == _r_pedge)
                        ) {
                        R_EmitCachedEdge();
                        _rLastVertValid = false;
                        continue;
                    }
                }
            }

            // assume it's cacheable
            _cacheOffset = (uint8_p)edge_p - (uint8_p)r_edges;
            _r_leftclipped = _r_rightclipped = false;
            R_ClipEdge(
                &r_pcurrentvertbase[_r_pedge->v16[0]],
                &r_pcurrentvertbase[_r_pedge->v16[1]],
                pclip);
            _r_pedge->cachededgeoffset = _cacheOffset;

            if (_r_leftclipped)  _makeLeftEdge = true;
            if (_r_rightclipped) _makeRightEdge = true;
            _rLastVertValid = true;
        }
        else {
            lindex = -lindex;
            _r_pedge = &pedges[lindex];
            if (!inSubModel)  // if the edge is cached, we can just reuse the edge
                if ((_r_pedge->cachededgeoffset & FULLY_CLIPPED_CACHED) &&
                    ((_r_pedge->cachededgeoffset & FRAMECOUNT_MASK) == r_framecount)
                    ) {
                    _rLastVertValid = false;
                    continue;
                }
                else    // it's cached if the cached edge is valid and is owned by this mEdge_t
                    if ((((uintptr_t)edge_p - (uintptr_t)r_edges) > _r_pedge->cachededgeoffset) &&
                        (((Edge_p)((uintptr_t)r_edges + _r_pedge->cachededgeoffset))->owner == _r_pedge)
                        ) {
                        R_EmitCachedEdge();
                        _rLastVertValid = false;
                        continue;
                    }

            // assume it's cacheable
            _cacheOffset = (uint8_p)edge_p - (uint8_p)r_edges;
            _r_leftclipped = _r_rightclipped = false;
            R_ClipEdge(
                &r_pcurrentvertbase[_r_pedge->v16[1]],
                &r_pcurrentvertbase[_r_pedge->v16[0]],
                pclip);
            _r_pedge->cachededgeoffset = _cacheOffset;

            if (_r_leftclipped)  _makeLeftEdge = true;
            if (_r_rightclipped) _makeRightEdge = true;
            _rLastVertValid = true; /* + */
        }
    }

    // if there was a clip off the left edge, add that edge too
    // FIXME: faster to do in screen space?
    // FIXME: share clipped edges?
    if (_makeLeftEdge) {
        _r_pedge = &_tEdge;
        _rLastVertValid = false;
        R_ClipEdge(&_r_leftexit, &_r_leftenter, pclip->next);
    }

    // if there was a clip off the right edge, get the right _r_nearzi
    if (_makeRightEdge) {
        _r_pedge = &_tEdge;
        _rLastVertValid = false;
        _r_nearzionly = true;
        R_ClipEdge(&_r_rightexit, &_r_rightenter, view_clipplanes[1].next);
    }

    // if no edges made it out, return without posting the surface
    if (!_r_emitted)
        return;

    r_polycount++;

    pSurface->data = (TypeLess_ptr)fa;
    pSurface->nearzi = _r_nearzi;
    pSurface->flags = fa->flags;
    pSurface->insubmodel = inSubModel;
    pSurface->spanstate = notInSpan;
    pSurface->entity = currententity;
    pSurface->key = r_currentkey++;
    pSurface->spans = NULL;

    mPlane_p pplane = fa->plane;
    // FIXME: cache this?
    vec3_t p_normal = TransformVector(pplane->normal);
    // FIXME: cache this?
    float distinv = 1.0f / (pplane->dist - DotProduct(modelorg, pplane->normal));

    pSurface->d_zistepu = p_normal.x * xscaleinv * distinv;
    pSurface->d_zistepv = -p_normal.y * yscaleinv * distinv;
    pSurface->d_ziorigin = p_normal.z * distinv -
        xcenter * pSurface->d_zistepu -
        ycenter * pSurface->d_zistepv;

    pSurface++;
}


/*
================
R_RenderBmodelFace
================
*/
void R_RenderBmodelFace(bEdge_p pedges, mSurface_p psurf) {
    static mEdge_t _tEdge;

    if (pSurface >= pSurf_max) {    // skip out if no more surfs
        r_outofsurfaces++;
        return;
    }

    if ((edge_p + psurf->numedges + 4) >= edge_max) {   // ditto if not enough edges left, or switch to auxedges if possible
        r_outofedges += psurf->numedges;
        return;
    }

    c_faceclip++;

    // this is a dummy to give the caching mechanism someplace to write to
    _r_pedge = &_tEdge;

    ClipPlane_p pclip = NULL;   // set up clip planes

    AliasClipFlags_f mask = ALIAS_BOTTOM_CLIP;
    for (int i = 3; i >= 0; i--, mask >>= 1) {
        if (r_clipflags & mask) {
            view_clipplanes[i].next = pclip;
            pclip = &view_clipplanes[i];
        }
    }

    // push the edges through
    _r_emitted = 0;
    _r_nearzi = 0;
    _r_nearzionly = false;
    _makeLeftEdge = _makeRightEdge = false;
    // FIXME: keep clipped bmodel edges in clockwise order so last vertex caching
    // can be used?
    _rLastVertValid = false;

    for (; pedges; pedges = pedges->pnext) {
        _r_leftclipped = _r_rightclipped = false;
        R_ClipEdge(pedges->v2[0], pedges->v2[1], pclip);

        if (_r_leftclipped)  _makeLeftEdge = true;
        if (_r_rightclipped) _makeRightEdge = true;
    }

    // if there was a clip off the left edge, add that edge too
    // FIXME: faster to do in screen space?
    // FIXME: share clipped edges?
    if (_makeLeftEdge) {
        _r_pedge = &_tEdge;
        R_ClipEdge(&_r_leftexit, &_r_leftenter, pclip->next);
    }

    // if there was a clip off the right edge, get the right _r_nearzi
    if (_makeRightEdge) {
        _r_pedge = &_tEdge;
        _r_nearzionly = true;
        R_ClipEdge(&_r_rightexit, &_r_rightenter, view_clipplanes[1].next);
    }

    // if no edges made it out, return without posting the surface
    if (!_r_emitted)
        return;

    r_polycount++;

    pSurface->data = (TypeLess_ptr)psurf;
    pSurface->nearzi = _r_nearzi;
    pSurface->flags = psurf->flags;
    pSurface->insubmodel = true;
    pSurface->spanstate = notInSpan;
    pSurface->entity = currententity;
    pSurface->key = r_currentbkey;
    pSurface->spans = NULL;

    mPlane_p pplane = psurf->plane;
    // FIXME: cache this?
    vec3_t p_normal = TransformVector(pplane->normal);
    // FIXME: cache this?
    float distinv = 1.0f / (pplane->dist - DotProduct(modelorg, pplane->normal));

    pSurface->d_zistepu = p_normal.x * xscaleinv * distinv;
    pSurface->d_zistepv = -p_normal.y * yscaleinv * distinv;
    pSurface->d_ziorigin = p_normal.z * distinv -
        xcenter * pSurface->d_zistepu -
        ycenter * pSurface->d_zistepv;

    pSurface++;
}


/*
================
R_RenderPoly
================
*/
void R_RenderPoly(mSurface_p fa, AliasClipFlags_f clipflags) {
    mVertex_t   verts[2][100]; //FIXME: do real number
    PolyVert_t  pverts[100]; //FIXME: do real number, safely

    // FIXME: clean this up and make it faster
    // FIXME: guard against running out of vertices

    // set up clip planes
    ClipPlane_p pclip = NULL;
    uint32_t mask = 0x08;
    for (int i = 3; i >= 0; i--, mask >>= 1) {
        if (clipflags & mask) {
            view_clipplanes[i].next = pclip;
            pclip = &view_clipplanes[i];
        }
    }

    // reconstruct the polygon
    // FIXME: these should be precalculated and loaded off disk
    mEdge_p pedges = currententity->model->edges;
    int lnumverts = fa->numedges;
    int vertpage = 0;

    for (int i = 0; i < lnumverts; i++) {
        int lindex = currententity->model->surfedges[fa->firstedge + i];

        if (lindex > 0) {
            _r_pedge = &pedges[lindex];
            verts[0][i] = r_pcurrentvertbase[_r_pedge->v16[0]];
        }
        else {
            _r_pedge = &pedges[-lindex];
            verts[0][i] = r_pcurrentvertbase[_r_pedge->v16[1]];
        }
    }

    // clip the polygon, done if not visible
    while (pclip) {
        int lastvert = lnumverts - 1;
        float lastdist = DotProduct(verts[vertpage][lastvert].position,
            pclip->normal) - pclip->dist;

        bool visible = false;
        int newverts = 0;
        int newpage = vertpage ^ 1;

        for (int i = 0; i < lnumverts; i++) {
            float dist = DotProduct(verts[vertpage][i].position, pclip->normal) -
                pclip->dist;

            if ((lastdist > 0) != (dist > 0)) {
                float frac = dist / (dist - lastdist);

                verts[newpage][newverts].position = VectorMA(
                    verts[vertpage][i].position,
                    frac, VectorSubtract(
                        verts[vertpage][lastvert].position,
                        verts[vertpage][i].position
                    )
                );
                newverts++;
            }

            if (dist >= 0) {
                verts[newpage][newverts] = verts[vertpage][i];
                newverts++;
                visible = true;
            }

            lastvert = i;
            lastdist = dist;
        }

        if (!visible || (newverts < 3))
            return;

        lnumverts = newverts;
        vertpage ^= 1;
        pclip = pclip->next;
    }

    // transform and project, remembering the z values at the vertices and
    // _r_nearzi, and extract the s and t coordinates at the vertices
    mPlane_p pplane = fa->plane;

    int s_axis, t_axis;
    switch (pplane->type) {
    default:    // compilator warning fix
    case PLANE_X:
    case PLANE_ANYX:    s_axis = 1; t_axis = 2;     break;
    case PLANE_Y:
    case PLANE_ANYY:    s_axis = 0; t_axis = 2;     break;
    case PLANE_Z:
    case PLANE_ANYZ:    s_axis = 0; t_axis = 1;     break;
    }

    _r_nearzi = 0;

    for (int i = 0; i < lnumverts; i++) {
        // transform and project
        vec3_t local = VectorSubtract(verts[vertpage][i].position, modelorg);
        vec3_t transformed = TransformVector(local);

        ClampLessThen(&transformed.z, NEAR_CLIP);

        float lzi = 1.0 / transformed.z;

        if (lzi > _r_nearzi) // for mipmap finding
            _r_nearzi = lzi;

        // FIXME: build x/yscale into transform?
        {
            float scale = xscale * lzi;
            float u = (xcenter + scale * transformed.x);
            ClampInRange(r_refdef.fvrectx_adj, &u, r_refdef.fvrectright_adj);
            pverts[i].u = u;
        }

        {
            float scale = yscale * lzi;
            float v = (ycenter - scale * transformed.y);
            ClampInRange(r_refdef.fvrecty_adj, &v, r_refdef.fvrectbottom_adj);
            pverts[i].v = v;
        }
        pverts[i].zi = lzi;
        pverts[i].s = verts[vertpage][i].position.v[s_axis];
        pverts[i].t = verts[vertpage][i].position.v[t_axis];
    }

    // build the polygon descriptor, including fa, _r_nearzi, and u, v, s, t, and z for each vertex
    r_polydesc.numverts = lnumverts;
    r_polydesc.nearzi = _r_nearzi;
    r_polydesc.pcurrentface = fa;
    r_polydesc.pverts = pverts;

    // draw the polygon
    D_DrawPoly();
}


/*
================
R_ZDrawSubmodelPolys
================
*/
void R_ZDrawSubmodelPolys(Model_p pmodel) {
    mSurface_p psurf = &pmodel->surfaces[pmodel->firstModelSurface];
    int numsurfaces = pmodel->numModelSurfaces;

    for (int i = 0; i < numsurfaces; i++, psurf++) {
        // find which side of the node we are on
        mPlane_p pplane = psurf->plane;

        float dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

        // draw the polygon
        if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
            (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON))) {
            // FIXME: use bounding-box-based frustum clipping info?
            R_RenderPoly(psurf, 15);
        }
    }
}

