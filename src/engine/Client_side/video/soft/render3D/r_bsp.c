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

#include "r_local.h"
#include "host.h"
#include "console.h"
#include "Surface.h"

//
// current entity info
//
r_Entity_p  currententity;
vec3_t  modelorg;       // modelorg is the viewpoint reletive to the currently rendering entity
vec3_t  base_modelorg;  // TODO: move to transform code
vec3_t  r_entorigin;    // the currently rendering entity in world coordinates
mat3_t  entity_rotation;
int     r_currentbkey;

typedef enum {
    touchessolid,
    drawnode,
    nodrawnode
} solidstate_t;

#define MAX_BMODEL_VERTS 500   // 6K
mVertex_t bverts[MAX_BMODEL_VERTS];
static mVertex_p _pbVerts;
static int       _numbVerts;

#define MAX_BMODEL_EDGES 1000  // 12K
bEdge_t bedges[MAX_BMODEL_EDGES];
static bEdge_p _pbEdges;
static int     _numbEges;

static mVertex_p _pFrontEnter;
static mVertex_p _pFrontExit;
static bool _makeClippedEdge;


//===========================================================================

/*
================
R_EntityRotate
================
*/
void R_EntityRotate(vec3_p vec) {
    vec3_t tvec = *vec;
    for (int i = 0; i < VECT_DIM; i++)
        vec->v[i] = DotProduct(entity_rotation.rows[i], tvec);
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
    float angle = currententity->pose.aim.yaw;
    angle = angle * M_PI * 2 / 360;
    float s = sin(angle);
    float c = cos(angle);


    mat3_t temp1 = {
            .m = {
                {   c,    s, 0.0f},
                {  -s,    c, 0.0f},
                {0.0f, 0.0f, 1.0f}
            }
    };
    // pitch
    angle = currententity->pose.aim.pitch;
    angle = angle * M_PI * 2 / 360;
    s = sin(angle);
    c = cos(angle);


    mat3_t temp2 = {
        .m = {
            {    c, 0.0f,   -s},
            { 0.0f, 1.0f, 0.0f},
            {    s, 0.0f,    c}
        }
    };


    mat3_t temp3;
    R_ConcatRotations(&temp2, &temp1, &temp3);

    // roll
    angle = currententity->pose.aim.roll;
    angle = angle * M_PI * 2 / 360;
    s = sin(angle);
    c = cos(angle);

    temp1 = (mat3_t){
        .m = {
            { 1.0f, 0.0f, 0.0f},
            { 0.0f,    c,    s},
            { 0.0f,   -s,    c}
        }
    };

    R_ConcatRotations(&temp1, &temp3, &entity_rotation);

    //
    // rotate modelorg and the transformation matrix
    //
    R_EntityRotate(&modelorg);

    R_EntityRotate(&BS.forward);
    R_EntityRotate(&BS.right);
    R_EntityRotate(&BS.up);

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
    _makeClippedEdge = false;

    // transform the BSP plane into model space
    // FIXME: cache these?
    mPlane_p splitplane = pnode->plane;
    mPlane_t tplane = {
        .dist =
            splitplane->dist -
            DotProduct(r_entorigin, splitplane->normal)
    };
    for (int i = 0; i < VECT_DIM; i++)
        tplane.normal.v[i] = DotProduct(entity_rotation.rows[i], splitplane->normal);

    // clip edges to BSP plane
    for (; pedges; pedges = pnextedge) {
        pnextedge = pedges->pnext;

        // set the status for the last point as the previous point
        // FIXME: cache this stuff somehow?
        mVertex_p plastvert = pedges->v2[0];
        float lastdist =
            DotProduct(plastvert->position, tplane.normal) -
            tplane.dist;
        int lastside = (lastdist > 0) ? 0 : 1;
        mVertex_p pvert = pedges->v2[1];
        float dist = DotProduct(pvert->position, tplane.normal) - tplane.dist;
        int side = (dist > 0) ? 0 : 1;

        if (side != lastside) {
            // clipped
            if (_numbVerts >= MAX_BMODEL_VERTS)
                return;

            // generate the clipped vertex
            float frac = lastdist / (lastdist - dist);
            mVertex_p ptvert = &_pbVerts[_numbVerts++];

            // Linear interpolation: pt = plast + frac * (pvert - plast)
            ptvert->position = VectorMA(plastvert->position,
                frac, VectorSubtract(pvert->position, plastvert->position)
            );

            // split into two edges, one on each side, and remember entering
            // and exiting points
            // FIXME: share the clip edge by having a winding direction flag?
            if (_numbEges >= (MAX_BMODEL_EDGES - 1)) {
                Con_Printf("Out of edges for bmodel\n");
                return;
            }

            bEdge_p ptedge = &_pbEdges[_numbEges];
            ptedge->pnext = psideedges[lastside];
            psideedges[lastside] = ptedge;
            ptedge->v2[0] = plastvert;
            ptedge->v2[1] = ptvert;

            ptedge = &_pbEdges[_numbEges + 1];
            ptedge->pnext = psideedges[side];
            psideedges[side] = ptedge;
            ptedge->v2[0] = ptvert;
            ptedge->v2[1] = pvert;

            _numbEges += 2;

            if (side == 0) {
                // entering for front, exiting for back
                _pFrontEnter = ptvert;
                _makeClippedEdge = true;
            }
            else {
                _pFrontExit = ptvert;
                _makeClippedEdge = true;
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
    if (_makeClippedEdge) {
        if (_numbEges >= (MAX_BMODEL_EDGES - 2)) {
            Con_Printf("Out of edges for bmodel\n");
            return;
        }

        bEdge_p ptedge = &_pbEdges[_numbEges];
        ptedge->pnext = psideedges[0];
        psideedges[0] = ptedge;
        ptedge->v2[0] = _pFrontExit;
        ptedge->v2[1] = _pFrontEnter;

        ptedge = &_pbEdges[_numbEges + 1];
        ptedge->pnext = psideedges[1];
        psideedges[1] = ptedge;
        ptedge->v2[0] = _pFrontEnter;
        ptedge->v2[1] = _pFrontExit;

        _numbEges += 2;
    }

    // draw or recurse further
    for (int i = 0; i < 2; i++) {
        if (psideedges[i]) {
            // draw if we've reached a non-solid leaf, done if all that's left is a
            // solid leaf, and continue down the tree if it's not a leaf
            mNode_p pn = pnode->children[i];

            // we're done with this branch if the node or leaf isn't in the PVS
            if (pn->visframe == r_visframecount) {
                if ((pn->contents < CONTENTS_NODE) &&
                    (pn->contents != CONTENTS_SOLID)
                    ) {
                    r_currentbkey = ((mLeaf_p)pn)->key;
                    R_RenderBmodelFace(psideedges[i], psurf);
                }
                else {
                    R_RecursiveClipBPoly(
                        psideedges[i],
                        pnode->children[i],
                        psurf
                    );
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
    mSurface_p psurf = &pmodel->surfaces[pmodel->firstModelSurface];
    int numsurfaces = pmodel->numModelSurfaces;
    mEdge_p pedges = pmodel->edges;

    for (int i = 0; i < numsurfaces; i++, psurf++) {
        // find which side of the node we are on
        mPlane_p pplane = psurf->plane;

        vec_t dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

        // draw the polygon
        if (
            (
                (psurf->flags & SURF_PLANEBACK) &&
                (dot < -BACKFACE_EPSILON)
                ) ||
            (
                !(psurf->flags & SURF_PLANEBACK) &&
                (dot > BACKFACE_EPSILON)
                )
            ) {
            // FIXME: use bounding-box-based frustum clipping info?

            // copy the edges to bedges, flipping if necessary so always
            // clockwise winding
            // FIXME: if edges and vertices get caches, these assignments must move
            // outside the loop, and overflow checking must be done here
            _pbVerts = bverts;
            _pbEdges = bedges;
            _numbVerts = _numbEges = 0;

            if (psurf->numedges > 0) {
                bEdge_p pbedge = &bedges[_numbEges];
                _numbEges += psurf->numedges;

                int j = 0;
                for (; j < psurf->numedges; j++) {
                    int lindex = pmodel->surfedges[psurf->firstedge + j];

                    if (lindex > 0) {
                        mEdge_p pedge = &pedges[lindex];
                        pbedge[j].v2[0] = &r_pcurrentvertbase[pedge->v16[0]];
                        pbedge[j].v2[1] = &r_pcurrentvertbase[pedge->v16[1]];
                    }
                    else {
                        lindex = -lindex;
                        mEdge_p pedge = &pedges[lindex];
                        pbedge[j].v2[0] = &r_pcurrentvertbase[pedge->v16[1]];
                        pbedge[j].v2[1] = &r_pcurrentvertbase[pedge->v16[0]];
                    }

                    pbedge[j].pnext = &pbedge[j + 1];
                }
                pbedge[j - 1].pnext = NULL; // mark end of edges
                R_RecursiveClipBPoly(pbedge, currententity->topnode, psurf);
            }
            else {
                Host_SysError("no edges in bmodel");
            }
        }
    }
}


/*
================
R_DrawSubmodelPolygons
================
*/
void R_DrawSubmodelPolygons(Model_p pmodel, AliasClipFlags_f clipflags) {
    // FIXME: use bounding-box-based frustum clipping info?
    mSurface_p psurf = &pmodel->surfaces[pmodel->firstModelSurface];
    int numsurfaces = pmodel->numModelSurfaces;

    for (int i = 0; i < numsurfaces; i++, psurf++) {
        // find which side of the node we are on
        mPlane_p pplane = psurf->plane;
        vec_t dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

        // draw the polygon
        if (
            (
                (psurf->flags & SURF_PLANEBACK) &&
                (dot < -BACKFACE_EPSILON)
                ) ||
            (!
                (psurf->flags & SURF_PLANEBACK) &&
                (dot > BACKFACE_EPSILON))
            ) {
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

void R_RecursiveWorldNode(mNode_p node, ClipFlag_t clipflags) {
    if ((node->contents == CONTENTS_SOLID) ||  // solid
        (node->visframe != r_visframecount)
        )   return;

    // cull the clipping planes if not trivial accept
    // FIXME: the compiler is doing a lousy job of optimizing here; it could be
    // twice as fast in ASM
    if (clipflags) {
        for (int i = 0; i < 4; i++) {
            if (!(clipflags & (1 << i)))
                continue; // don't need to clip against it

            // generate accept and reject points
            // FIXME: do with fast look-ups or integer tests based on the sign bit of the floating point values
            {
                vec3_t rejectpt = VecXYZ(
                    node->bb.v[pfrustum_indexes[i][X_AX]],
                    node->bb.v[pfrustum_indexes[i][Y_AX]],
                    node->bb.v[pfrustum_indexes[i][Z_AX]]
                );
                double d = DotProduct(rejectpt, view_clipplanes[i].normal) - view_clipplanes[i].dist;
                if (d <= 0.f)     return;
            }
            {
                vec3_t acceptpt = VecXYZ(
                    node->bb.v[pfrustum_indexes[i][3 + X_AX]],
                    node->bb.v[pfrustum_indexes[i][3 + Y_AX]],
                    node->bb.v[pfrustum_indexes[i][3 + Z_AX]]
                );
                double d = DotProduct(acceptpt, view_clipplanes[i].normal) - view_clipplanes[i].dist;
                if (d >= 0.f)     clipflags &= ~(1 << i); // node is entirely on screen
            }
        }
    }

    // if a leaf node, draw stuff
    if (node->contents < CONTENTS_NODE) {
        mLeaf_p pleaf = (mLeaf_p)node;

        mSurface_ar mark = pleaf->firstmarksurface;
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

        return; // recursive exit
    }
    else {
        // node is just a decision point, so go down the apropriate sides
        // find which side of the node we are on
        mPlane_p plane = node->plane;
        double  dot;
        switch (plane->type) {
        case PLANE_X: { dot = modelorg.x - plane->dist; } break;
        case PLANE_Y: { dot = modelorg.y - plane->dist; } break;
        case PLANE_Z: { dot = modelorg.z - plane->dist; } break;
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
                                    pbtofpolys[numbtofpolys++] = (btofpoly_t){
                                        .clipflags = clipflags,
                                        .psurf = surf
                                    };
                                }
                            }
                            else { R_RenderPoly(surf, clipflags); }
                        }
                        else { R_RenderFace(surf, clipflags); }
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
                                    pbtofpolys[numbtofpolys++] = (btofpoly_t){
                                        .clipflags = clipflags,
                                        .psurf = surf
                                    };
                                }
                            }
                            else { R_RenderPoly(surf, clipflags); }
                        }
                        else { R_RenderFace(surf, clipflags); }
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
#include "mem_placement.h"
btofpoly_t _bTofPolys[MAX_BTOFPOLYS] PLACE_TO_SDRAM;
void R_RenderWorld() {

    pbtofpolys = _bTofPolys;

    currententity = &cl_entities[0];
    modelorg = r_origin;
    Model_p clmodel = currententity->model;
    r_pcurrentvertbase = clmodel->vertexes;

    R_RecursiveWorldNode(clmodel->nodes, 15); // TODO: it make overflow and corrupt 'bool configRestored' and 'bool serialAvailable'

    // if the driver wants the polygons back to front, play the visible ones back in that order
    if (r_worldpolysbacktofront) {
        for (int i = numbtofpolys - 1; i >= 0; i--)
            R_RenderPoly(_bTofPolys[i].psurf, _bTofPolys[i].clipflags);
    }
}


