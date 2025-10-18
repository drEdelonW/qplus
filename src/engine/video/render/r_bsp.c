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
// r_bsp.c

#include "quakedef.h"
#include "r_local.h"

//
// current entity info
//
qboolean    insubmodel;
r_Entity_p  currententity;
vec3_t      modelorg, base_modelorg;
// modelorg is the viewpoint reletive to
// the currently rendering entity
vec3_t      r_entorigin; // the currently rendering entity in world
// coordinates

float   entity_rotation[3][3];
vec3_t  r_worldmodelorg;
int     r_currentbkey;

typedef enum { touchessolid, drawnode, nodrawnode } solidstate_t;

#define MAX_BMODEL_VERTS 500   // 6K
#define MAX_BMODEL_EDGES 1000  // 12K

static mVertex_p    pbverts;
static bEdge_p      pbedges;
static int          numbverts, numbedges;
static mVertex_p    pfrontenter, pfrontexit;
static qboolean     makeclippededge;


//===========================================================================

/*
================
R_EntityRotate
================
*/
void R_EntityRotate(vec3_t vec) {
    vec3_t tvec;
    VectorCopy(vec, tvec);
    for (int i = 0; i < VECT_DIM; i++)
        vec[i] = DotProduct(entity_rotation[i], tvec);
}


/*
================
R_RotateBmodel
================
*/
void R_RotateBmodel() {

    // TODO: should use a look-up table
    // TODO: should really be stored with the entity instead of being reconstructed
    // TODO: could cache lazily, stored in the entity
    // TODO: share work with R_SetUpAliasTransform

    // yaw
    float angle = currententity->angles[YAW];
    angle = angle * M_PI * 2 / 360;
    float s = sin(angle);
    float c = cos(angle);

    float temp1[3][3];
    temp1[0][0] = c;    temp1[0][1] = s;    temp1[0][2] = 0;
    temp1[1][0] = -s;   temp1[1][1] = c;    temp1[1][2] = 0;
    temp1[2][0] = 0;    temp1[2][1] = 0;    temp1[2][2] = 1;

    // pitch
    angle = currententity->angles[PITCH];
    angle = angle * M_PI * 2 / 360;
    s = sin(angle);
    c = cos(angle);

    float temp2[3][3];
    temp2[0][0] = c;    temp2[0][1] = 0;    temp2[0][2] = -s;
    temp2[1][0] = 0;    temp2[1][1] = 1;    temp2[1][2] = 0;
    temp2[2][0] = s;    temp2[2][1] = 0;    temp2[2][2] = c;

    float temp3[3][3];
    R_ConcatRotations(temp2, temp1, temp3);

    // roll
    angle = currententity->angles[ROLL];
    angle = angle * M_PI * 2 / 360;
    s = sin(angle);
    c = cos(angle);

    temp1[0][0] = 1;    temp1[0][1] = 0;    temp1[0][2] = 0;
    temp1[1][0] = 0;    temp1[1][1] = c;    temp1[1][2] = s;
    temp1[2][0] = 0;    temp1[2][1] = -s;   temp1[2][2] = c;

    R_ConcatRotations(temp1, temp3, entity_rotation);

    //
    // rotate modelorg and the transformation matrix
    //
    R_EntityRotate(modelorg);
    R_EntityRotate(vpn);
    R_EntityRotate(vright);
    R_EntityRotate(vup);

    R_TransformFrustum();
}


/*
================
R_RecursiveClipBPoly
================
*/
void R_RecursiveClipBPoly(bEdge_p pedges, mNode_p pnode, mSurface_p psurf) {
    bEdge_p psideedges[2], pnextedge;

    psideedges[0] = psideedges[1] = NULL;
    makeclippededge = false;

    // transform the BSP plane into model space
    // FIXME: cache these?
    mPlane_p splitplane = pnode->plane;
    mPlane_t tplane = {
        .dist =
            splitplane->dist -
            DotProduct(r_entorigin, splitplane->normal)
    };
    for (int i = 0; i < VECT_DIM; i++)
        tplane.normal[i] = DotProduct(entity_rotation[i], splitplane->normal);

    // clip edges to BSP plane
    for (; pedges; pedges = pnextedge) {
        pnextedge = pedges->pnext;

        // set the status for the last point as the previous point
        // FIXME: cache this stuff somehow?
        mVertex_p plastvert = pedges->v[0];
        float lastdist =
            DotProduct(plastvert->position, tplane.normal) -
            tplane.dist;
        int lastside = (lastdist > 0) ? 0 : 1;
        mVertex_p pvert = pedges->v[1];
        float dist = DotProduct(pvert->position, tplane.normal) - tplane.dist;
        int side = (dist > 0) ? 0 : 1;

        if (side != lastside) {
            // clipped
            if (numbverts >= MAX_BMODEL_VERTS)
                return;

            // generate the clipped vertex
            float frac = lastdist / (lastdist - dist);
            mVertex_p ptvert = &pbverts[numbverts++];
            ptvert->position[0] =
                plastvert->position[0] +
                frac * (pvert->position[0] -
                    plastvert->position[0]);
            ptvert->position[1] =
                plastvert->position[1] +
                frac * (pvert->position[1] -
                    plastvert->position[1]);
            ptvert->position[2] =
                plastvert->position[2] +
                frac * (pvert->position[2] -
                    plastvert->position[2]);

            // split into two edges, one on each side, and remember entering
            // and exiting points
            // FIXME: share the clip edge by having a winding direction flag?
            if (numbedges >= (MAX_BMODEL_EDGES - 1)) {
                Con_Printf("Out of edges for bmodel\n");
                return;
            }

            bEdge_p ptedge = &pbedges[numbedges];
            ptedge->pnext = psideedges[lastside];
            psideedges[lastside] = ptedge;
            ptedge->v[0] = plastvert;
            ptedge->v[1] = ptvert;

            ptedge = &pbedges[numbedges + 1];
            ptedge->pnext = psideedges[side];
            psideedges[side] = ptedge;
            ptedge->v[0] = ptvert;
            ptedge->v[1] = pvert;

            numbedges += 2;

            if (side == 0) {
                // entering for front, exiting for back
                pfrontenter = ptvert;
                makeclippededge = true;
            }
            else {
                pfrontexit = ptvert;
                makeclippededge = true;
            }
        }
        else {
            // add the edge to the appropriate side
            pedges->pnext = psideedges[side];
            psideedges[side] = pedges;
        }
    }

    // if anything was clipped, reconstitute and add the edges along the clip
    // plane to both sides (but in opposite directions)
    if (makeclippededge) {
        if (numbedges >= (MAX_BMODEL_EDGES - 2)) {
            Con_Printf("Out of edges for bmodel\n");
            return;
        }

        bEdge_p ptedge = &pbedges[numbedges];
        ptedge->pnext = psideedges[0];
        psideedges[0] = ptedge;
        ptedge->v[0] = pfrontexit;
        ptedge->v[1] = pfrontenter;

        ptedge = &pbedges[numbedges + 1];
        ptedge->pnext = psideedges[1];
        psideedges[1] = ptedge;
        ptedge->v[0] = pfrontenter;
        ptedge->v[1] = pfrontexit;

        numbedges += 2;
    }

    // draw or recurse further
    for (int i = 0; i < 2; i++) {
        if (psideedges[i]) {
            // draw if we've reached a non-solid leaf, done if all that's left is a
            // solid leaf, and continue down the tree if it's not a leaf
            mNode_p pn = pnode->children[i];

            // we're done with this branch if the node or leaf isn't in the PVS
            if (pn->visframe == r_visframecount) {
                if ((pn->contents < 0) &&
                    (pn->contents != CONTENTS_SOLID)) {
                    r_currentbkey = ((mLeaf_p)pn)->key;
                    R_RenderBmodelFace(psideedges[i], psurf);
                }
                else {
                    R_RecursiveClipBPoly(
                        psideedges[i],
                        pnode->children[i],
                        psurf);
                }
            }
        }
    }
}


/*
================
R_DrawSolidClippedSubmodelPolygons
================
*/
void R_DrawSolidClippedSubmodelPolygons(Model_p pmodel) {
    // FIXME: use bounding-box-based frustum clipping info?
    mSurface_p psurf = &pmodel->surfaces[pmodel->firstmodelsurface];
    int numsurfaces = pmodel->nummodelsurfaces;
    mEdge_p pedges = pmodel->edges;

    for (int i = 0; i < numsurfaces; i++, psurf++) {
        // find which side of the node we are on
        mPlane_p pplane = psurf->plane;

        vec_t dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

        // draw the polygon
        if (((psurf->flags & SURF_PLANEBACK) &&
            (dot < -BACKFACE_EPSILON)) ||
            (!(psurf->flags & SURF_PLANEBACK) &&
                (dot > BACKFACE_EPSILON))) {
            // FIXME: use bounding-box-based frustum clipping info?

            // copy the edges to bedges, flipping if necessary so always
            // clockwise winding
            // FIXME: if edges and vertices get caches, these assignments must move
            // outside the loop, and overflow checking must be done here
            mVertex_t bverts[MAX_BMODEL_VERTS];
            pbverts = bverts;
            bEdge_t bedges[MAX_BMODEL_EDGES];
            pbedges = bedges;
            numbverts = numbedges = 0;

            if (psurf->numedges > 0) {
                bEdge_p pbedge = &bedges[numbedges];
                numbedges += psurf->numedges;

                int j = 0;
                for (; j < psurf->numedges; j++) {
                    int lindex = pmodel->surfedges[psurf->firstedge + j];

                    if (lindex > 0) {
                        mEdge_p pedge = &pedges[lindex];
                        pbedge[j].v[0] = &r_pcurrentvertbase[pedge->v[0]];
                        pbedge[j].v[1] = &r_pcurrentvertbase[pedge->v[1]];
                    }
                    else {
                        lindex = -lindex;
                        mEdge_p pedge = &pedges[lindex];
                        pbedge[j].v[0] = &r_pcurrentvertbase[pedge->v[1]];
                        pbedge[j].v[1] = &r_pcurrentvertbase[pedge->v[0]];
                    }

                    pbedge[j].pnext = &pbedge[j + 1];
                }
                pbedge[j - 1].pnext = NULL; // mark end of edges
                R_RecursiveClipBPoly(pbedge, currententity->topnode, psurf);
            }
            else {
                Sys_Error("no edges in bmodel");
            }
        }
    }
}


/*
================
R_DrawSubmodelPolygons
================
*/
void R_DrawSubmodelPolygons(Model_p pmodel, int clipflags) {
    // FIXME: use bounding-box-based frustum clipping info?
    mSurface_p psurf = &pmodel->surfaces[pmodel->firstmodelsurface];
    int numsurfaces = pmodel->nummodelsurfaces;

    for (int i = 0; i < numsurfaces; i++, psurf++) {
        // find which side of the node we are on
        mPlane_p pplane = psurf->plane;
        vec_t dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

        // draw the polygon
        if (((psurf->flags & SURF_PLANEBACK) &&
            (dot < -BACKFACE_EPSILON)) ||
            (!(psurf->flags & SURF_PLANEBACK) &&
                (dot > BACKFACE_EPSILON))) {
            r_currentkey = ((mLeaf_p)currententity->topnode)->key;

            // FIXME: use bounding-box-based frustum clipping info?
            R_RenderFace(psurf, clipflags);
        }
    }
}


/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode(mNode_p node, int clipflags) {
    if ((node->contents == CONTENTS_SOLID) ||  // solid
        (node->visframe != r_visframecount))
        return;

    // cull the clipping planes if not trivial accept
    // FIXME: the compiler is doing a lousy job of optimizing here; it could be
    //  twice as fast in ASM
    if (clipflags) {
        for (int i = 0; i < 4; i++) {
            if (!(clipflags & (1 << i)))
                continue; // don't need to clip against it

            // generate accept and reject points
            // FIXME: do with fast look-ups or integer tests based on the sign bit
            // of the floating point values

            int* pindex = pfrustum_indexes[i];

            {
                vec3_t rejectpt = {
                    (float)node->minmaxs[pindex[0]],
                    (float)node->minmaxs[pindex[1]],
                    (float)node->minmaxs[pindex[2]],
                };
                double d = DotProduct(rejectpt, view_clipplanes[i].normal);
                d -= view_clipplanes[i].dist;

                if (d <= 0)
                    return;
            }

            {
                vec3_t acceptpt = {
                    (float)node->minmaxs[pindex[3 + 0]],
                    (float)node->minmaxs[pindex[3 + 1]],
                    (float)node->minmaxs[pindex[3 + 2]]
                };
                double d = DotProduct(acceptpt, view_clipplanes[i].normal);
                d -= view_clipplanes[i].dist;

                if (d >= 0)
                    clipflags &= ~(1 << i); // node is entirely on screen
            }
        }
    }

    // if a leaf node, draw stuff
    if (node->contents < 0) {
        mLeaf_p pleaf = (mLeaf_p)node;

        mSurface_p* mark = pleaf->firstmarksurface;
        int c = pleaf->nummarksurfaces;

        if (c) {
            do {
                (*mark)->visframe = r_framecount;
                mark++;
            } while (--c);
        }

        // deal with model fragments in this leaf
        if (pleaf->efrags) {
            R_StoreEfrags(&pleaf->efrags);
        }

        pleaf->key = r_currentkey;
        r_currentkey++;  // all bmodels in a leaf share the same key
    }
    else {
        // node is just a decision point, so go down the apropriate sides

        // find which side of the node we are on
        mPlane_p plane = node->plane;

        double  dot;
        switch (plane->type) {
        case PLANE_X: dot = modelorg[0] - plane->dist;  break;
        case PLANE_Y: dot = modelorg[1] - plane->dist;  break;
        case PLANE_Z: dot = modelorg[2] - plane->dist;  break;
        default: dot = DotProduct(modelorg, plane->normal) - plane->dist; break;
        }

        int side = (dot >= 0) ? 0 : 1;

        // recurse down the children, front side first
        R_RecursiveWorldNode(node->children[side], clipflags);

        // draw stuff
        int c = node->numsurfaces;
        if (c) {
            mSurface_p surf = cl.worldmodel->surfaces + node->firstsurface;

            if (dot < -BACKFACE_EPSILON) {
                do {
                    if ((surf->flags & SURF_PLANEBACK) &&
                        (surf->visframe == r_framecount)) {
                        if (r_drawpolys) {
                            if (r_worldpolysbacktofront) {
                                if (numbtofpolys < MAX_BTOFPOLYS) {
                                    pbtofpolys[numbtofpolys].clipflags =
                                        clipflags;
                                    pbtofpolys[numbtofpolys].psurf = surf;
                                    numbtofpolys++;
                                }
                            }
                            else {
                                R_RenderPoly(surf, clipflags);
                            }
                        }
                        else {
                            R_RenderFace(surf, clipflags);
                        }
                    }

                    surf++;
                } while (--c);
            }
            else if (dot > BACKFACE_EPSILON) {
                do {
                    if (!(surf->flags & SURF_PLANEBACK) &&
                        (surf->visframe == r_framecount)) {
                        if (r_drawpolys) {
                            if (r_worldpolysbacktofront) {
                                if (numbtofpolys < MAX_BTOFPOLYS) {
                                    pbtofpolys[numbtofpolys].clipflags =
                                        clipflags;
                                    pbtofpolys[numbtofpolys].psurf = surf;
                                    numbtofpolys++;
                                }
                            }
                            else {
                                R_RenderPoly(surf, clipflags);
                            }
                        }
                        else {
                            R_RenderFace(surf, clipflags);
                        }
                    }

                    surf++;
                } while (--c);
            }

            // all surfaces on the same node share the same sequence number
            r_currentkey++;
        }

        // recurse down the back side
        R_RecursiveWorldNode(node->children[!side], clipflags);
    }
}



/*
================
R_RenderWorld
================
*/
void R_RenderWorld() {
    btofpoly_t btofpolys[MAX_BTOFPOLYS];

    pbtofpolys = btofpolys;

    currententity = &cl_entities[0];
    VectorCopy(r_origin, modelorg);
    Model_p clmodel = currententity->model;
    r_pcurrentvertbase = clmodel->vertexes;

    R_RecursiveWorldNode(clmodel->nodes, 15);

    // if the driver wants the polygons back to front, play the visible ones back
    // in that order
    if (r_worldpolysbacktofront) {
        for (int i = numbtofpolys - 1; i >= 0; i--) {
            R_RenderPoly(btofpolys[i].psurf, btofpolys[i].clipflags);
        }
    }
}


