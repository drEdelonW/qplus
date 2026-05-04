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

#define MAXLEFTCLIPEDGES  (100)

// !!! if these are changed, they must be changed in asm_draw.h too !!!
#define FULLY_CLIPPED_CACHED (0x80000000)
#define FRAMECOUNT_MASK   (0x7FFFFFFF)

static uint32_t _cacheOffset;
int             c_faceclip;     // number of faces clipped
zPointDesc_t    r_zpointdesc;
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
    float u, v, lzi;
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
        float_p world = &pv0->position[0];

        // transform and project
        vec3_t local;   VectorSubtract(world, modelorg, local);
        vec3_t transformed; TransformVector(local, transformed);

        if (transformed[2] < NEAR_CLIP)
            transformed[2] = NEAR_CLIP;

        em.lzi = 1.0 / transformed[2];

        // FIXME: build x/yscale into transform?
        {
            float scale = xscale * em.lzi;
            em.u = (xcenter + scale * transformed[0]);
            if (em.u < r_refdef.fvrectx_adj)
                em.u = r_refdef.fvrectx_adj;
            if (em.u > r_refdef.fvrectright_adj)
                em.u = r_refdef.fvrectright_adj;
        }

        {
            float scale = yscale * em.lzi;
            em.v = (ycenter - scale * transformed[1]);
            if (em.v < r_refdef.fvrecty_adj)
                em.v = r_refdef.fvrecty_adj;
            if (em.v > r_refdef.fvrectbottom_adj)
                em.v = r_refdef.fvrectbottom_adj;
        }
        em.ceilv = (int)ceil(em.v);
    }

    float_p world = &pv1->position[0];

    // transform and project
    vec3_t local; VectorSubtract(world, modelorg, local);
    vec3_t transformed; TransformVector(local, transformed);

    if (transformed[2] < NEAR_CLIP)
        transformed[2] = NEAR_CLIP;

    _r.lzi = 1.0 / transformed[2];

    {
        float scale = xscale * _r.lzi;
        _r.u = (xcenter + scale * transformed[0]);
        if (_r.u < r_refdef.fvrectx_adj)        _r.u = r_refdef.fvrectx_adj;
        if (_r.u > r_refdef.fvrectright_adj)    _r.u = r_refdef.fvrectright_adj;
    }

    {
        float scale = yscale * _r.lzi;
        _r.v = (ycenter - scale * transformed[1]);
        if (_r.v < r_refdef.fvrecty_adj)        _r.v = r_refdef.fvrecty_adj;
        if (_r.v > r_refdef.fvrectbottom_adj)   _r.v = r_refdef.fvrectbottom_adj;
    }

    if (_r.lzi > em.lzi)      em.lzi = _r.lzi;

    if (em.lzi > _r_nearzi) // for mipmap finding
        _r_nearzi = em.lzi;

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
    if (side == 0) {
        // trailing edge (go from p1 to p2)
        v = em.ceilv;
        v2 = _r.ceilv - 1;

        edge->surfs[0] = surface_p - surfaces;
        edge->surfs[1] = 0;

        u_step = ((_r.u - em.u) / (_r.v - em.v));
        u = em.u + ((float)v - em.v) * u_step;
    }
    else {
        // leading edge (go from p2 to p1)
        v2 = em.ceilv - 1;
        v = _r.ceilv;

        edge->surfs[0] = 0;
        edge->surfs[1] = surface_p - surfaces;

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
    if (edge->u < r_refdef.vrect_x_adj_shift20)     edge->u = r_refdef.vrect_x_adj_shift20;
    if (edge->u > r_refdef.vrectright_adj_shift20)  edge->u = r_refdef.vrectright_adj_shift20;

    //
    // sort the edge in normally
    //
    int u_check = edge->u;
    if (edge->surfs[0])
        u_check++; // sort trailers after leaders

    if (!newedges[v] || newedges[v]->u >= u_check) {
        edge->next = newedges[v];
        newedges[v] = edge;
    }
    else {
        Edge_p pcheck = newedges[v];
        while (pcheck->next && pcheck->next->u < u_check)
            pcheck = pcheck->next;
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

            if (d0 >= 0) {
                // point 0 is unclipped
                if (d1 >= 0) {
                    // both points are unclipped
                    continue;
                }

                // only point 1 is clipped

                // we don't cache clipped edges
                _cacheOffset = 0x7FFFFFFF;

                float f = d0 / (d0 - d1);
                mVertex_t clipvert;
                clipvert.position[0] = pv0->position[0] + f * (pv1->position[0] - pv0->position[0]);
                clipvert.position[1] = pv0->position[1] + f * (pv1->position[1] - pv0->position[1]);
                clipvert.position[2] = pv0->position[2] + f * (pv1->position[2] - pv0->position[2]);

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
            else {
                // point 0 is clipped
                if (d1 < 0) {
                    // both points are clipped
                    // we do cache fully clipped edges
                    if (!_r_leftclipped)
                        _cacheOffset = FULLY_CLIPPED_CACHED |
                        (r_framecount & FRAMECOUNT_MASK);
                    return;
                }

                // only point 0 is clipped
                _rLastVertValid = false;

                // we don't cache partially clipped edges
                _cacheOffset = 0x7FFFFFFF;

                float f = d0 / (d0 - d1);
                mVertex_t clipvert;
                for (int i = 0; i < VECT_DIM; i++)
                    clipvert.position[i] = pv0->position[i] + f * (pv1->position[i] - pv0->position[i]);

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

    // add the edge
    R_EmitEdge(pv0, pv1);
}

#endif // !id386


/*
================
R_EmitCachedEdge
================
*/
void R_EmitCachedEdge() {
    Edge_p pedge_t = (Edge_p)((uintptr_t)r_edges + (uintptr_t)_r_pedge->cachededgeoffset);

    if (!pedge_t->surfs[0])     pedge_t->surfs[0] = surface_p - surfaces;
    else                        pedge_t->surfs[1] = surface_p - surfaces;

    if (pedge_t->nearzi > _r_nearzi) // for mipmap finding
        _r_nearzi = pedge_t->nearzi;

    _r_emitted = 1;
}


/*
================
R_RenderFace
================
*/
void R_RenderFace(mSurface_p fa, int clipflags) {
    static mEdge_t _tEdge;

    // skip out if no more surfs
    if ((surface_p) >= surf_max) {
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

    uint32_t mask = 0x08;
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
            if (!insubmodel) {
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
                &r_pcurrentvertbase[_r_pedge->v[0]],
                &r_pcurrentvertbase[_r_pedge->v[1]],
                pclip);
            _r_pedge->cachededgeoffset = _cacheOffset;

            if (_r_leftclipped)  _makeLeftEdge = true;
            if (_r_rightclipped) _makeRightEdge = true;
            _rLastVertValid = true;
        }
        else {
            lindex = -lindex;
            _r_pedge = &pedges[lindex];
            // if the edge is cached, we can just reuse the edge
            if (!insubmodel) {
                if ((_r_pedge->cachededgeoffset & FULLY_CLIPPED_CACHED) &&
                    ((_r_pedge->cachededgeoffset & FRAMECOUNT_MASK) == r_framecount)) {
                    _rLastVertValid = false;
                    continue;
                }
                else {
                    // it's cached if the cached edge is valid and is owned
                    // by this mEdge_t
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
                &r_pcurrentvertbase[_r_pedge->v[1]],
                &r_pcurrentvertbase[_r_pedge->v[0]],
                pclip);
            _r_pedge->cachededgeoffset = _cacheOffset;

            if (_r_leftclipped)  _makeLeftEdge = true;
            if (_r_rightclipped) _makeRightEdge = true;
            _rLastVertValid = true;
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

    surface_p->data = (TypeLess_ptr)fa;
    surface_p->nearzi = _r_nearzi;
    surface_p->flags = fa->flags;
    surface_p->insubmodel = insubmodel;
    surface_p->spanstate = notInSpan;
    surface_p->entity = currententity;
    surface_p->key = r_currentkey++;
    surface_p->spans = NULL;

    mPlane_p pplane = fa->plane;
    // FIXME: cache this?
    vec3_t p_normal; TransformVector(pplane->normal, p_normal);
    // FIXME: cache this?
    float distinv = 1.0f / (pplane->dist - DotProduct(modelorg, pplane->normal));

    surface_p->d_zistepu = p_normal[0] * xscaleinv * distinv;
    surface_p->d_zistepv = -p_normal[1] * yscaleinv * distinv;
    surface_p->d_ziorigin = p_normal[2] * distinv -
        xcenter * surface_p->d_zistepu -
        ycenter * surface_p->d_zistepv;

    //JDC VectorCopy (r_worldmodelorg, surface_p->modelorg);
    surface_p++;
}


/*
================
R_RenderBmodelFace
================
*/
void R_RenderBmodelFace(bEdge_p pedges, mSurface_p psurf) {
    static mEdge_t _tEdge;

    // skip out if no more surfs
    if (surface_p >= surf_max) {
        r_outofsurfaces++;
        return;
    }

    // ditto if not enough edges left, or switch to auxedges if possible
    if ((edge_p + psurf->numedges + 4) >= edge_max) {
        r_outofedges += psurf->numedges;
        return;
    }

    c_faceclip++;

    // this is a dummy to give the caching mechanism someplace to write to
    _r_pedge = &_tEdge;

    // set up clip planes
    ClipPlane_p pclip = NULL;

    uint32_t mask = 0x08;
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
        R_ClipEdge(pedges->v[0], pedges->v[1], pclip);

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

    surface_p->data = (TypeLess_ptr)psurf;
    surface_p->nearzi = _r_nearzi;
    surface_p->flags = psurf->flags;
    surface_p->insubmodel = true;
    surface_p->spanstate = notInSpan;
    surface_p->entity = currententity;
    surface_p->key = r_currentbkey;
    surface_p->spans = NULL;

    mPlane_p pplane = psurf->plane;
    // FIXME: cache this?
    vec3_t p_normal; TransformVector(pplane->normal, p_normal);
    // FIXME: cache this?
    float distinv = 1.0f / (pplane->dist - DotProduct(modelorg, pplane->normal));

    surface_p->d_zistepu = p_normal[0] * xscaleinv * distinv;
    surface_p->d_zistepv = -p_normal[1] * yscaleinv * distinv;
    surface_p->d_ziorigin = p_normal[2] * distinv -
        xcenter * surface_p->d_zistepu -
        ycenter * surface_p->d_zistepv;

    //JDC VectorCopy (r_worldmodelorg, surface_p->modelorg);
    surface_p++;
}


/*
================
R_RenderPoly
================
*/
void R_RenderPoly(mSurface_p fa, int clipflags) {
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
            verts[0][i] = r_pcurrentvertbase[_r_pedge->v[0]];
        }
        else {
            _r_pedge = &pedges[-lindex];
            verts[0][i] = r_pcurrentvertbase[_r_pedge->v[1]];
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
                verts[newpage][newverts].position[0] = verts[vertpage][i].position[0] + ((verts[vertpage][lastvert].position[0] - verts[vertpage][i].position[0]) * frac);
                verts[newpage][newverts].position[1] = verts[vertpage][i].position[1] + ((verts[vertpage][lastvert].position[1] - verts[vertpage][i].position[1]) * frac);
                verts[newpage][newverts].position[2] = verts[vertpage][i].position[2] + ((verts[vertpage][lastvert].position[2] - verts[vertpage][i].position[2]) * frac);
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
        vec3_t local; VectorSubtract(verts[vertpage][i].position, modelorg, local);
        vec3_t transformed; TransformVector(local, transformed);

        if (transformed[2] < NEAR_CLIP)
            transformed[2] = NEAR_CLIP;

        float lzi = 1.0 / transformed[2];

        if (lzi > _r_nearzi) // for mipmap finding
            _r_nearzi = lzi;

        // FIXME: build x/yscale into transform?
        {
            float scale = xscale * lzi;
            float u = (xcenter + scale * transformed[0]);
            if (u < r_refdef.fvrectx_adj)       u = r_refdef.fvrectx_adj;
            if (u > r_refdef.fvrectright_adj)   u = r_refdef.fvrectright_adj;
            pverts[i].u = u;
        }

        {
            float scale = yscale * lzi;
            float v = (ycenter - scale * transformed[1]);
            if (v < r_refdef.fvrecty_adj)       v = r_refdef.fvrecty_adj;
            if (v > r_refdef.fvrectbottom_adj)  v = r_refdef.fvrectbottom_adj;
            pverts[i].v = v;
        }
        pverts[i].zi = lzi;
        pverts[i].s = verts[vertpage][i].position[s_axis];
        pverts[i].t = verts[vertpage][i].position[t_axis];
    }

    // build the polygon descriptor, including fa, _r_nearzi, and u, v, s, t, and z
    // for each vertex
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

