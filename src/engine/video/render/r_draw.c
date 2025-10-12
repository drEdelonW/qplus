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

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"	// FIXME: shouldn't need to include this

#define MAXLEFTCLIPEDGES		100

// !!! if these are changed, they must be changed in asm_draw.h too !!!
#define FULLY_CLIPPED_CACHED	0x80000000
#define FRAMECOUNT_MASK			0x7FFFFFFF

uint32_t        cacheoffset;
int             c_faceclip;					// number of faces clipped
zpointdesc_t    r_zpointdesc;
polydesc_t      r_polydesc;

clipplane_p entity_clipplanes;
clipplane_t	view_clipplanes[4];
clipplane_t	world_clipplanes[16];

medge_p r_pedge;

qboolean        r_leftclipped, r_rightclipped;
static qboolean	makeleftedge, makerightedge;
qboolean        r_nearzionly;

int sintable[SIN_BUFFER_SIZE];
int intsintable[SIN_BUFFER_SIZE];

mvertex_t	r_leftenter, r_leftexit;
mvertex_t	r_rightenter, r_rightexit;

typedef struct {
    float	u, v;
    int		ceilv;
} evert_t;

int     r_emitted;
float   r_nearzi;
float   r_u1, r_v1, r_lzi1;
int     r_ceilv1;

qboolean	r_lastvertvalid;

#if	!id386

/*
================
R_EmitEdge
================
*/
void R_EmitEdge(mvertex_p pv0, mvertex_p pv1) {
    int		v, v2, ceilv0;
    float	scale, lzi0, u0, v0;

    if (r_lastvertvalid) {
        u0 = r_u1;
        v0 = r_v1;
        lzi0 = r_lzi1;
        ceilv0 = r_ceilv1;
    }
    else {
        float_p world = &pv0->position[0];

        // transform and project
        vec3_t	local, transformed;
        VectorSubtract(world, modelorg, local);
        TransformVector(local, transformed);

        if (transformed[2] < NEAR_CLIP)
            transformed[2] = NEAR_CLIP;

        lzi0 = 1.0 / transformed[2];

        // FIXME: build x/yscale into transform?
        scale = xscale * lzi0;
        u0 = (xcenter + scale * transformed[0]);
        if (u0 < r_refdef.fvrectx_adj)
            u0 = r_refdef.fvrectx_adj;
        if (u0 > r_refdef.fvrectright_adj)
            u0 = r_refdef.fvrectright_adj;

        scale = yscale * lzi0;
        v0 = (ycenter - scale * transformed[1]);
        if (v0 < r_refdef.fvrecty_adj)
            v0 = r_refdef.fvrecty_adj;
        if (v0 > r_refdef.fvrectbottom_adj)
            v0 = r_refdef.fvrectbottom_adj;

        ceilv0 = (int)ceil(v0);
    }

    float_p world = &pv1->position[0];

    // transform and project
    vec3_t	local, transformed;
    VectorSubtract(world, modelorg, local);
    TransformVector(local, transformed);

    if (transformed[2] < NEAR_CLIP)
        transformed[2] = NEAR_CLIP;

    r_lzi1 = 1.0 / transformed[2];

    scale = xscale * r_lzi1;
    r_u1 = (xcenter + scale * transformed[0]);
    if (r_u1 < r_refdef.fvrectx_adj)
        r_u1 = r_refdef.fvrectx_adj;
    if (r_u1 > r_refdef.fvrectright_adj)
        r_u1 = r_refdef.fvrectright_adj;

    scale = yscale * r_lzi1;
    r_v1 = (ycenter - scale * transformed[1]);
    if (r_v1 < r_refdef.fvrecty_adj)
        r_v1 = r_refdef.fvrecty_adj;
    if (r_v1 > r_refdef.fvrectbottom_adj)
        r_v1 = r_refdef.fvrectbottom_adj;

    if (r_lzi1 > lzi0)
        lzi0 = r_lzi1;

    if (lzi0 > r_nearzi)	// for mipmap finding
        r_nearzi = lzi0;

    // for right edges, all we want is the effect on 1/z
    if (r_nearzionly)
        return;

    r_emitted = 1;

    r_ceilv1 = (int)ceil(r_v1);


    // create the edge
    if (ceilv0 == r_ceilv1) {
        // we cache unclipped horizontal edges as fully clipped
        if (cacheoffset != 0x7FFFFFFF) {
            cacheoffset = FULLY_CLIPPED_CACHED |
                (r_framecount & FRAMECOUNT_MASK);
        }

        return;		// horizontal edge
    }

    int side = ceilv0 > r_ceilv1;

    edge_ptr edge = edge_p++;

    edge->owner = r_pedge;

    edge->nearzi = lzi0;

    float u, u_step;
    if (side == 0) {
        // trailing edge (go from p1 to p2)
        v = ceilv0;
        v2 = r_ceilv1 - 1;

        edge->surfs[0] = surface_p - surfaces;
        edge->surfs[1] = 0;

        u_step = ((r_u1 - u0) / (r_v1 - v0));
        u = u0 + ((float)v - v0) * u_step;
    }
    else {
        // leading edge (go from p2 to p1)
        v2 = ceilv0 - 1;
        v = r_ceilv1;

        edge->surfs[0] = 0;
        edge->surfs[1] = surface_p - surfaces;

        u_step = ((u0 - r_u1) / (v0 - r_v1));
        u = r_u1 + ((float)v - r_v1) * u_step;
    }

    edge->u_step = u_step * 0x100000;
    edge->u = u * 0x100000 + 0xFFFFF;

    // we need to do this to avoid stepping off the edges if a very nearly
    // horizontal edge is less than epsilon above a scan, and numeric error causes
    // it to incorrectly extend to the scan, and the extension of the line goes off
    // the edge of the screen
    // FIXME: is this actually needed?
    if (edge->u < r_refdef.vrect_x_adj_shift20)
        edge->u = r_refdef.vrect_x_adj_shift20;
    if (edge->u > r_refdef.vrectright_adj_shift20)
        edge->u = r_refdef.vrectright_adj_shift20;

    //
    // sort the edge in normally
    //
    int u_check = edge->u;
    if (edge->surfs[0])
        u_check++;	// sort trailers after leaders

    if (!newedges[v] || newedges[v]->u >= u_check) {
        edge->next = newedges[v];
        newedges[v] = edge;
    }
    else {
        edge_ptr pcheck = newedges[v];
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
void R_ClipEdge(mvertex_p pv0, mvertex_p pv1, clipplane_p clip) {
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
                cacheoffset = 0x7FFFFFFF;

                float f = d0 / (d0 - d1);
                mvertex_t clipvert;
                clipvert.position[0] = pv0->position[0] +
                    f * (pv1->position[0] - pv0->position[0]);
                clipvert.position[1] = pv0->position[1] +
                    f * (pv1->position[1] - pv0->position[1]);
                clipvert.position[2] = pv0->position[2] +
                    f * (pv1->position[2] - pv0->position[2]);

                if (clip->leftedge) {
                    r_leftclipped = true;
                    r_leftexit = clipvert;
                }
                else if (clip->rightedge) {
                    r_rightclipped = true;
                    r_rightexit = clipvert;
                }

                R_ClipEdge(pv0, &clipvert, clip->next);
                return;
            }
            else {
                // point 0 is clipped
                if (d1 < 0) {
                    // both points are clipped
                    // we do cache fully clipped edges
                    if (!r_leftclipped)
                        cacheoffset = FULLY_CLIPPED_CACHED |
                        (r_framecount & FRAMECOUNT_MASK);
                    return;
                }

                // only point 0 is clipped
                r_lastvertvalid = false;

                // we don't cache partially clipped edges
                cacheoffset = 0x7FFFFFFF;

                float f = d0 / (d0 - d1);
                mvertex_t clipvert;
                clipvert.position[0] = pv0->position[0] +
                    f * (pv1->position[0] - pv0->position[0]);
                clipvert.position[1] = pv0->position[1] +
                    f * (pv1->position[1] - pv0->position[1]);
                clipvert.position[2] = pv0->position[2] +
                    f * (pv1->position[2] - pv0->position[2]);

                if (clip->leftedge) {
                    r_leftclipped = true;
                    r_leftenter = clipvert;
                }
                else if (clip->rightedge) {
                    r_rightclipped = true;
                    r_rightenter = clipvert;
                }

                R_ClipEdge(&clipvert, pv1, clip->next);
                return;
            }
        } while ((clip = clip->next) != NULL);
    }

    // add the edge
    R_EmitEdge(pv0, pv1);
}

#endif	// !id386


/*
================
R_EmitCachedEdge
================
*/
void R_EmitCachedEdge() {
    edge_ptr pedge_t = (edge_ptr)((uint32_t)r_edges + r_pedge->cachededgeoffset);

    if (!pedge_t->surfs[0])
        pedge_t->surfs[0] = surface_p - surfaces;
    else
        pedge_t->surfs[1] = surface_p - surfaces;

    if (pedge_t->nearzi > r_nearzi)	// for mipmap finding
        r_nearzi = pedge_t->nearzi;

    r_emitted = 1;
}


/*
================
R_RenderFace
================
*/
void R_RenderFace(msurface_p fa, int clipflags) {
    medge_t tedge;

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
    clipplane_p pclip = NULL;

    uint32_t mask = 0x08;
    for (int i = 3; i >= 0; i--, mask >>= 1) {
        if (clipflags & mask) {
            view_clipplanes[i].next = pclip;
            pclip = &view_clipplanes[i];
        }
    }

    // push the edges through
    r_emitted = 0;
    r_nearzi = 0;
    r_nearzionly = false;
    makeleftedge = makerightedge = false;
    medge_p pedges = currententity->model->edges;
    r_lastvertvalid = false;

    for (int i = 0; i < fa->numedges; i++) {
        int lindex = currententity->model->surfedges[fa->firstedge + i];

        if (lindex > 0) {
            r_pedge = &pedges[lindex];

            // if the edge is cached, we can just reuse the edge
            if (!insubmodel) {
                if (r_pedge->cachededgeoffset & FULLY_CLIPPED_CACHED) {
                    if ((r_pedge->cachededgeoffset & FRAMECOUNT_MASK) ==
                        r_framecount) {
                        r_lastvertvalid = false;
                        continue;
                    }
                }
                else {
                    if ((((uint32_t)edge_p - (uint32_t)r_edges) >
                        r_pedge->cachededgeoffset) &&
                        (((edge_ptr)((uint32_t)r_edges +
                            r_pedge->cachededgeoffset))->owner == r_pedge)) {
                        R_EmitCachedEdge();
                        r_lastvertvalid = false;
                        continue;
                    }
                }
            }

            // assume it's cacheable
            cacheoffset = (uint8_p)edge_p - (uint8_p)r_edges;
            r_leftclipped = r_rightclipped = false;
            R_ClipEdge(&r_pcurrentvertbase[r_pedge->v[0]],
                &r_pcurrentvertbase[r_pedge->v[1]],
                pclip);
            r_pedge->cachededgeoffset = cacheoffset;

            if (r_leftclipped)
                makeleftedge = true;
            if (r_rightclipped)
                makerightedge = true;
            r_lastvertvalid = true;
        }
        else {
            lindex = -lindex;
            r_pedge = &pedges[lindex];
            // if the edge is cached, we can just reuse the edge
            if (!insubmodel) {
                if (r_pedge->cachededgeoffset & FULLY_CLIPPED_CACHED) {
                    if ((r_pedge->cachededgeoffset & FRAMECOUNT_MASK) ==
                        r_framecount) {
                        r_lastvertvalid = false;
                        continue;
                    }
                }
                else {
                    // it's cached if the cached edge is valid and is owned
                    // by this medge_t
                    if ((((uint32_t)edge_p - (uint32_t)r_edges) >
                        r_pedge->cachededgeoffset) &&
                        (((edge_ptr)((uint32_t)r_edges +
                            r_pedge->cachededgeoffset))->owner == r_pedge)) {
                        R_EmitCachedEdge();
                        r_lastvertvalid = false;
                        continue;
                    }
                }
            }

            // assume it's cacheable
            cacheoffset = (uint8_p)edge_p - (uint8_p)r_edges;
            r_leftclipped = r_rightclipped = false;
            R_ClipEdge(&r_pcurrentvertbase[r_pedge->v[1]],
                &r_pcurrentvertbase[r_pedge->v[0]],
                pclip);
            r_pedge->cachededgeoffset = cacheoffset;

            if (r_leftclipped)
                makeleftedge = true;
            if (r_rightclipped)
                makerightedge = true;
            r_lastvertvalid = true;
        }
    }

    // if there was a clip off the left edge, add that edge too
    // FIXME: faster to do in screen space?
    // FIXME: share clipped edges?
    if (makeleftedge) {
        r_pedge = &tedge;
        r_lastvertvalid = false;
        R_ClipEdge(&r_leftexit, &r_leftenter, pclip->next);
    }

    // if there was a clip off the right edge, get the right r_nearzi
    if (makerightedge) {
        r_pedge = &tedge;
        r_lastvertvalid = false;
        r_nearzionly = true;
        R_ClipEdge(&r_rightexit, &r_rightenter, view_clipplanes[1].next);
    }

    // if no edges made it out, return without posting the surface
    if (!r_emitted)
        return;

    r_polycount++;

    surface_p->data = (typeless_ptr)fa;
    surface_p->nearzi = r_nearzi;
    surface_p->flags = fa->flags;
    surface_p->insubmodel = insubmodel;
    surface_p->spanstate = 0;
    surface_p->entity = currententity;
    surface_p->key = r_currentkey++;
    surface_p->spans = NULL;

    mplane_p pplane = fa->plane;
    // FIXME: cache this?
    vec3_t      p_normal;
    TransformVector(pplane->normal, p_normal);
    // FIXME: cache this?
    float distinv = 1.0 / (pplane->dist - DotProduct(modelorg, pplane->normal));

    surface_p->d_zistepu = p_normal[0] * xscaleinv * distinv;
    surface_p->d_zistepv = -p_normal[1] * yscaleinv * distinv;
    surface_p->d_ziorigin = p_normal[2] * distinv -
        xcenter * surface_p->d_zistepu -
        ycenter * surface_p->d_zistepv;

    //JDC	VectorCopy (r_worldmodelorg, surface_p->modelorg);
    surface_p++;
}


/*
================
R_RenderBmodelFace
================
*/
void R_RenderBmodelFace(bedge_p pedges, msurface_p psurf) {
    static medge_t  tedge;

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
    r_pedge = &tedge;

    // set up clip planes
    clipplane_p pclip = NULL;

    uint32_t mask = 0x08;
    for (int i = 3; i >= 0; i--, mask >>= 1) {
        if (r_clipflags & mask) {
            view_clipplanes[i].next = pclip;
            pclip = &view_clipplanes[i];
        }
    }

    // push the edges through
    r_emitted = 0;
    r_nearzi = 0;
    r_nearzionly = false;
    makeleftedge = makerightedge = false;
    // FIXME: keep clipped bmodel edges in clockwise order so last vertex caching
    // can be used?
    r_lastvertvalid = false;

    for (; pedges; pedges = pedges->pnext) {
        r_leftclipped = r_rightclipped = false;
        R_ClipEdge(pedges->v[0], pedges->v[1], pclip);

        if (r_leftclipped)
            makeleftedge = true;
        if (r_rightclipped)
            makerightedge = true;
    }

    // if there was a clip off the left edge, add that edge too
    // FIXME: faster to do in screen space?
    // FIXME: share clipped edges?
    if (makeleftedge) {
        r_pedge = &tedge;
        R_ClipEdge(&r_leftexit, &r_leftenter, pclip->next);
    }

    // if there was a clip off the right edge, get the right r_nearzi
    if (makerightedge) {
        r_pedge = &tedge;
        r_nearzionly = true;
        R_ClipEdge(&r_rightexit, &r_rightenter, view_clipplanes[1].next);
    }

    // if no edges made it out, return without posting the surface
    if (!r_emitted)
        return;

    r_polycount++;

    surface_p->data = (typeless_ptr)psurf;
    surface_p->nearzi = r_nearzi;
    surface_p->flags = psurf->flags;
    surface_p->insubmodel = true;
    surface_p->spanstate = 0;
    surface_p->entity = currententity;
    surface_p->key = r_currentbkey;
    surface_p->spans = NULL;

    mplane_p pplane = psurf->plane;
    // FIXME: cache this?
    vec3_t		p_normal;
    TransformVector(pplane->normal, p_normal);
    // FIXME: cache this?
    float distinv = 1.0 / (pplane->dist - DotProduct(modelorg, pplane->normal));

    surface_p->d_zistepu = p_normal[0] * xscaleinv * distinv;
    surface_p->d_zistepv = -p_normal[1] * yscaleinv * distinv;
    surface_p->d_ziorigin = p_normal[2] * distinv -
        xcenter * surface_p->d_zistepu -
        ycenter * surface_p->d_zistepv;

    //JDC	VectorCopy (r_worldmodelorg, surface_p->modelorg);
    surface_p++;
}


/*
================
R_RenderPoly
================
*/
void R_RenderPoly(msurface_p fa, int clipflags) {
    int         s_axis, t_axis;
    mvertex_t   verts[2][100];	//FIXME: do real number
    polyvert_t  pverts[100];	//FIXME: do real number, safely

    // FIXME: clean this up and make it faster
    // FIXME: guard against running out of vertices

    s_axis = t_axis = 0;	// keep compiler happy

    // set up clip planes
    clipplane_p pclip = NULL;
    uint32_t mask = 0x08;
    for (int i = 3; i >= 0; i--, mask >>= 1) {
        if (clipflags & mask) {
            view_clipplanes[i].next = pclip;
            pclip = &view_clipplanes[i];
        }
    }

    // reconstruct the polygon
    // FIXME: these should be precalculated and loaded off disk
    medge_p pedges = currententity->model->edges;
    int lnumverts = fa->numedges;
    int vertpage = 0;

    for (int i = 0; i < lnumverts; i++) {
        int lindex = currententity->model->surfedges[fa->firstedge + i];

        if (lindex > 0) {
            r_pedge = &pedges[lindex];
            verts[0][i] = r_pcurrentvertbase[r_pedge->v[0]];
        }
        else {
            r_pedge = &pedges[-lindex];
            verts[0][i] = r_pcurrentvertbase[r_pedge->v[1]];
        }
    }

    // clip the polygon, done if not visible
    while (pclip) {
        int lastvert = lnumverts - 1;
        float lastdist = DotProduct(verts[vertpage][lastvert].position,
            pclip->normal) - pclip->dist;

        qboolean visible = false;
        int newverts = 0;
        int newpage = vertpage ^ 1;

        for (int i = 0; i < lnumverts; i++) {
            float dist = DotProduct(verts[vertpage][i].position, pclip->normal) -
                pclip->dist;

            if ((lastdist > 0) != (dist > 0)) {
                float frac = dist / (dist - lastdist);
                verts[newpage][newverts].position[0] =
                    verts[vertpage][i].position[0] +
                    ((verts[vertpage][lastvert].position[0] -
                        verts[vertpage][i].position[0]) * frac);
                verts[newpage][newverts].position[1] =
                    verts[vertpage][i].position[1] +
                    ((verts[vertpage][lastvert].position[1] -
                        verts[vertpage][i].position[1]) * frac);
                verts[newpage][newverts].position[2] =
                    verts[vertpage][i].position[2] +
                    ((verts[vertpage][lastvert].position[2] -
                        verts[vertpage][i].position[2]) * frac);
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
    // r_nearzi, and extract the s and t coordinates at the vertices
    mplane_p pplane = fa->plane;
    switch (pplane->type) {
    case PLANE_X:
    case PLANE_ANYX:
        s_axis = 1;
        t_axis = 2;
        break;
    case PLANE_Y:
    case PLANE_ANYY:
        s_axis = 0;
        t_axis = 2;
        break;
    case PLANE_Z:
    case PLANE_ANYZ:
        s_axis = 0;
        t_axis = 1;
        break;
    }

    r_nearzi = 0;

    for (int i = 0; i < lnumverts; i++) {
        // transform and project
        vec3_t local, transformed;
        VectorSubtract(verts[vertpage][i].position, modelorg, local);
        TransformVector(local, transformed);

        if (transformed[2] < NEAR_CLIP)
            transformed[2] = NEAR_CLIP;

        float lzi = 1.0 / transformed[2];

        if (lzi > r_nearzi)	// for mipmap finding
            r_nearzi = lzi;

        // FIXME: build x/yscale into transform?
        float scale;
        scale = xscale * lzi;
        float u = (xcenter + scale * transformed[0]);
        if (u < r_refdef.fvrectx_adj)
            u = r_refdef.fvrectx_adj;
        if (u > r_refdef.fvrectright_adj)
            u = r_refdef.fvrectright_adj;

        scale = yscale * lzi;
        float v = (ycenter - scale * transformed[1]);
        if (v < r_refdef.fvrecty_adj)
            v = r_refdef.fvrecty_adj;
        if (v > r_refdef.fvrectbottom_adj)
            v = r_refdef.fvrectbottom_adj;

        pverts[i].u = u;
        pverts[i].v = v;
        pverts[i].zi = lzi;
        pverts[i].s = verts[vertpage][i].position[s_axis];
        pverts[i].t = verts[vertpage][i].position[t_axis];
    }

    // build the polygon descriptor, including fa, r_nearzi, and u, v, s, t, and z
    // for each vertex
    r_polydesc.numverts = lnumverts;
    r_polydesc.nearzi = r_nearzi;
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
void R_ZDrawSubmodelPolys(model_p pmodel) {
    msurface_p psurf = &pmodel->surfaces[pmodel->firstmodelsurface];
    int numsurfaces = pmodel->nummodelsurfaces;

    for (int i = 0; i < numsurfaces; i++, psurf++) {
        // find which side of the node we are on
        mplane_p pplane = psurf->plane;

        float dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

        // draw the polygon
        if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
            (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON))) {
            // FIXME: use bounding-box-based frustum clipping info?
            R_RenderPoly(psurf, 15);
        }
    }
}

