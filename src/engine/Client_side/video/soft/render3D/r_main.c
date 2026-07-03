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
// r_main.c

#include "r_local.h"
#include "render.h"
#include "host.h"
#include "sys.h"
#include "sound.h"
#include "cmd.h"
#include "gamedefs.h"
#include "console.h"
#include "q_tools.h"
#include "z_hunk.h"
#include "Texture.h"
#include "qTime.h"

//define PASSAGES

TypeLess_ptr colormap;
vec3_t  viewlightvec;
aLight_t    r_viewlighting = { 128, 192, &viewlightvec };
float   r_time1;
int     r_numallocatededges;
bool    r_drawpolys;
bool    r_drawculledpolys;
bool    r_worldpolysbacktofront;
bool    r_recursiveaffinetriangles = true;
int     r_pixbytes = 1; // TODO: make enum
float   r_aliasuvscale = 1.0f;
int     r_outofsurfaces;
int     r_outofedges;

bool    r_dowarp, r_dowarpold, r_viewchanged;

int         numbtofpolys;
btofpoly_p  pbtofpolys;
mVertex_p   r_pcurrentvertbase;

int     c_surf;
int     r_maxsurfsseen, r_maxedgesseen;
int r_cnumsurfs;
bool    r_surfsonstack;
int     r_clipflags;

uint8_p     r_warpbuffer;
uint8_p     r_stack_start;

bool r_fov_greater_than_90;

//
// view origin
//


vec3_t r_origin;    // TODO: mx 3x4?

//
// screen size info
//
refdef_t r_refdef;
float  xcenter, ycenter;
float  xscale, yscale;
float  xscaleinv, yscaleinv;
float  xscaleshrink, yscaleshrink;

int  screenwidth;

float pixelAspect;
float screenAspect;
float verticalFieldOfView;
float xOrigin, yOrigin;

mPlane_t screenedge[4];

//
// refresh flags
//
int  r_framecount = 1; // so frame counts initialized to 0 don't match
int  r_visframecount;
int  d_spanpixcount;
int  r_polycount;
int  r_drawnpolycount;
int  r_wholepolycount;

#define VIEWMODNAME_LENGTH 256
char    viewmodname[VIEWMODNAME_LENGTH + 1];
int modcount;


mLeaf_p     r_viewleaf, r_oldviewleaf;

fixed8_t  d_lightstylevalue[256]; // 8.8 fraction of base light value

RealTime_t dp_time1, dp_time2;
RealTime_t db_time1, db_time2;
RealTime_t rw_time1, rw_time2;
RealTime_t se_time1, se_time2;
RealTime_t de_time1, de_time2;
RealTime_t dv_time1, dv_time2;

void R_MarkLeaves();
void CreatePassages();
void SetVisibilityByPassages();


/*
================
R_InitTurb
================
*/
void R_InitTurb() {
    for (int i = 0; i < (SIN_BUFFER_SIZE); i++) {
        sintable[i] = AMP + sin(i * 3.14159 * 2 / CYCLE) * AMP;
        intsintable[i] = AMP2 + sin(i * 3.14159 * 2 / CYCLE) * AMP2; // AMP2, not 20
    }
}

/*
===============
R_Init
===============
*/
void R_Init() {
    int  dummy;

    // get stack position so we can guess if we are going to overflow
    r_stack_start = (uint8_p)&dummy;

    R_InitTurb();

    Cmd_AddCommand("timerefresh", R_TimeRefresh_f);
    Cmd_AddCommand("pointfile", R_ReadPointFile_f);

    Cvar_RegisterVariable(&r_draworder);
    Cvar_RegisterVariable(&r_speeds);
    Cvar_RegisterVariable(&r_timegraph);
    Cvar_RegisterVariable(&r_graphheight);
    Cvar_RegisterVariable(&r_drawflat);
    Cvar_RegisterVariable(&r_ambient);
    Cvar_RegisterVariable(&r_clearcolor);
    Cvar_RegisterVariable(&r_waterwarp);
    Cvar_RegisterVariable(&r_fullbright);
    Cvar_RegisterVariable(&r_drawentities);
    Cvar_RegisterVariable(&r_drawviewmodel);
    Cvar_RegisterVariable(&r_drawworld);
    Cvar_RegisterVariable(&r_lightmap);
    Cvar_RegisterVariable(&r_dlightmap);
    Cvar_RegisterVariable(&r_aliasstats);
    Cvar_RegisterVariable(&r_dspeeds);
    Cvar_RegisterVariable(&r_reportsurfout);
    Cvar_RegisterVariable(&r_maxsurfs);
    Cvar_RegisterVariable(&r_numsurfs);
    Cvar_RegisterVariable(&r_reportedgeout);
    Cvar_RegisterVariable(&r_maxedges);
    Cvar_RegisterVariable(&r_numedges);
    Cvar_RegisterVariable(&r_aliastransbase);
    Cvar_RegisterVariable(&r_aliastransadj);

    Cvar_SetValue("r_maxedges", (float)NUMSTACKEDGES);
    Cvar_SetValue("r_maxsurfs", (float)NUMSTACKSURFACES);

    view_clipplanes[0].leftedge = true;
    view_clipplanes[1].rightedge = true;
    view_clipplanes[1].leftedge =
        view_clipplanes[2].leftedge =
        view_clipplanes[3].leftedge = false;
    view_clipplanes[0].rightedge =
        view_clipplanes[2].rightedge =
        view_clipplanes[3].rightedge = false;

    r_refdef.xOrigin = XCENTERING;
    r_refdef.yOrigin = YCENTERING;

    R_InitParticles();

    // TODO: collect 386-specific code in one place
#if id386
    Sys_MakeCodeWriteable(
        (int32_t)R_EdgeCodeStart,
        (int32_t)R_EdgeCodeEnd - (int32_t)R_EdgeCodeStart
    );
#endif // id386

    D_Init();
}

/*
===============
R_NewMap
===============
*/
void R_NewMap() {
    // clear out efrags in case the level hasn't been reloaded
    // FIXME: is this one short?
    for (int i = 0; i < cl.worldmodel->numleafs; i++)
        cl.worldmodel->leafs[i].efrags = NULL;

    r_viewleaf = NULL;
    R_ClearParticles();

    r_cnumsurfs = r_maxsurfs.value;

    if (r_cnumsurfs <= MINSURFACES)
        r_cnumsurfs = MINSURFACES;

    if (r_cnumsurfs > NUMSTACKSURFACES) {
        surfaces = Hunk_AllocName(r_cnumsurfs * sizeof(Surf_t), "surfaces");
        surface_p = surfaces;
        surf_max = &surfaces[r_cnumsurfs];
        r_surfsonstack = false;
        // surface 0 doesn't really exist; it's just a dummy because index 0
        // is used to indicate no edge attached to surface
        surfaces--;
        R_SurfacePatch();
    }
    else {
        r_surfsonstack = true;
    }

    r_maxedgesseen = 0;
    r_maxsurfsseen = 0;

    r_numallocatededges = r_maxedges.value;

    if (r_numallocatededges < MINEDGES)
        r_numallocatededges = MINEDGES;

    if (r_numallocatededges <= NUMSTACKEDGES) {
        auxedges = NULL;
    }
    else {
        auxedges = Hunk_AllocName(r_numallocatededges * sizeof(Edge_t),
            "edges");
    }

    r_dowarpold = false;
    r_viewchanged = false;
#ifdef PASSAGES
    CreatePassages();
#endif
}


/*
===============
R_ViewChanged

Called every time the vid structure or r_refdef changes.
Guaranteed to be called before the first refresh
===============
*/
void R_ViewChanged(vRect_p pvrect, int lineadj, float aspect) {
    r_viewchanged = true;

    R_SetVrect(pvrect, &r_refdef.vrect, lineadj);

    r_refdef.horizontalFieldOfView = 2.0 * tan(r_refdef.fov_x / 360 * M_PI);
    r_refdef.fvrectx = (float)r_refdef.vrect.x;
    r_refdef.fvrectx_adj = (float)r_refdef.vrect.x - 0.5;
    r_refdef.vrect_x_adj_shift20 = (r_refdef.vrect.x << 20) + (1 << 19) - 1;
    r_refdef.fvrecty = (float)r_refdef.vrect.y;
    r_refdef.fvrecty_adj = (float)r_refdef.vrect.y - 0.5;
    r_refdef.vrectright = r_refdef.vrect.x + r_refdef.vrect.width;
    r_refdef.vrectright_adj_shift20 = (r_refdef.vrectright << 20) + (1 << 19) - 1;
    r_refdef.fvrectright = (float)r_refdef.vrectright;
    r_refdef.fvrectright_adj = (float)r_refdef.vrectright - 0.5;
    r_refdef.vrectrightedge = (float)r_refdef.vrectright - 0.99;
    r_refdef.vrectbottom = r_refdef.vrect.y + r_refdef.vrect.height;
    r_refdef.fvrectbottom = (float)r_refdef.vrectbottom;
    r_refdef.fvrectbottom_adj = (float)r_refdef.vrectbottom - 0.5;

    r_refdef.aliasvrect.x = (int)(r_refdef.vrect.x * r_aliasuvscale);
    r_refdef.aliasvrect.y = (int)(r_refdef.vrect.y * r_aliasuvscale);
    r_refdef.aliasvrect.width = (int)(r_refdef.vrect.width * r_aliasuvscale);
    r_refdef.aliasvrect.height = (int)(r_refdef.vrect.height * r_aliasuvscale);
    r_refdef.aliasvrectright =
        r_refdef.aliasvrect.x +
        r_refdef.aliasvrect.width;
    r_refdef.aliasvrectbottom =
        r_refdef.aliasvrect.y +
        r_refdef.aliasvrect.height;

    pixelAspect = aspect;
    xOrigin = r_refdef.xOrigin;
    yOrigin = r_refdef.yOrigin;

    screenAspect = r_refdef.vrect.width * pixelAspect /
        r_refdef.vrect.height;
    // 320*200 1.0 pixelAspect = 1.6 screenAspect
    // 320*240 1.0 pixelAspect = 1.3333 screenAspect
    // proper 320*200 pixelAspect = 0.8333333

    verticalFieldOfView = r_refdef.horizontalFieldOfView / screenAspect;

    // values for perspective projection
    // if math were exact, the values would range from 0.5 to to range+0.5
    // hopefully they wll be in the 0.000001 to range+.999999 and truncate
    // the polygon rasterization will never render in the first row or column
    // but will definately render in the [range] row and column, so adjust the
    // buffer origin to get an exact edge to edge fill
    xcenter =
        ((float)r_refdef.vrect.width * XCENTERING) +
        r_refdef.vrect.x - 0.5;
    aliasxcenter = xcenter * r_aliasuvscale;
    ycenter =
        ((float)r_refdef.vrect.height * YCENTERING) +
        r_refdef.vrect.y - 0.5;
    aliasycenter = ycenter * r_aliasuvscale;

    xscale = r_refdef.vrect.width / r_refdef.horizontalFieldOfView;
    aliasxscale = xscale * r_aliasuvscale;
    xscaleinv = 1.0 / xscale;
    yscale = xscale * pixelAspect;
    aliasyscale = yscale * r_aliasuvscale;
    yscaleinv = 1.0 / yscale;
    xscaleshrink = (r_refdef.vrect.width - 6) / r_refdef.horizontalFieldOfView;
    yscaleshrink = xscaleshrink * pixelAspect;

    screenedge[0] = (mPlane_t){  // left side clip
        .normal = {
            .x = -1.0f / (xOrigin * r_refdef.horizontalFieldOfView),
            .y = 0.0f,
            .z = 1.0f
        },
        .type = PLANE_ANYZ
    };
    screenedge[1] = (mPlane_t){   // right side clip
        .normal = {
            .x = 1.0f / ((1.0f - xOrigin) * r_refdef.horizontalFieldOfView),
            .y = 0.0f,
            .z = 1.0f
        },
        .type = PLANE_ANYZ
    };
    screenedge[2] = (mPlane_t){  // top side clip
        .normal = {
            .x = 0.0f,
            .y = -1.0f / (yOrigin * verticalFieldOfView),
            .z = 1.0f
        },
        .type = PLANE_ANYZ
    };
    screenedge[3] = (mPlane_t){   // bottom side clip
        .normal = {
            .x = 0.0f,
            .y = 1.0f / ((1.0f - yOrigin) * verticalFieldOfView),
            .z = 1.0f
        },
        .type = PLANE_ANYZ
    };

    for (int i = 0; i < 4; i++)
        VectorNormalize(&screenedge[i].normal);

    float res_scale =
        sqrt(
            (double)(
                r_refdef.vrect.width * r_refdef.vrect.height) /
            (320.0 * 152.0)) *
        (2.0 / r_refdef.horizontalFieldOfView);
    r_aliastransition = r_aliastransbase.value * res_scale;
    r_resfudge = r_aliastransadj.value * res_scale;

    r_fov_greater_than_90 = (scr_fov.value > 90.0f);

    // TODO: collect 386-specific code in one place
#if id386
    if (r_pixbytes == 1) {
        Sys_MakeCodeWriteable(
            (int32_t)R_Surf8Start,
            (int32_t)R_Surf8End - (int32_t)R_Surf8Start
        );
        colormap = vid.colormap;
        R_Surf8Patch();
    }
    else {
        Sys_MakeCodeWriteable(
            (int32_t)R_Surf16Start,
            (int32_t)R_Surf16End - (int32_t)R_Surf16Start
        );
        colormap = vid.colormap16;
        R_Surf16Patch();
    }
#endif // id386

    D_ViewChanged();
}


/*
===============
R_MarkLeaves
===============
*/
uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model); // TODO: FIX include hell

void R_MarkLeaves() {
    if (r_oldviewleaf == r_viewleaf)
        return;

    r_visframecount++;
    r_oldviewleaf = r_viewleaf;

    uint8_p vis = Mod_LeafPVS(r_viewleaf, cl.worldmodel);

    for (int i = 0; i < cl.worldmodel->numleafs; i++) {
        if (vis[EIGHTH(i)] & (1 << (i & 7))) {
            mNode_p node = (mNode_p)&cl.worldmodel->leafs[i + 1];
            do {
                if (node->visframe == r_visframecount)
                    break;
                node->visframe = r_visframecount;
                node = node->parent;
            } while (node);
        }
    }
}


/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList() {
    // FIXME: remove and do real lighting
    if (!r_drawentities.value)      return;

    for (int i = 0; i < cl_numvisedicts; i++) {
        currententity = cl_visedicts[i];

        if (currententity == &cl_entities[cl.viewentity])
            continue; // don't draw the player

        switch (currententity->model->type) {
        case mod_sprite: {
            r_entorigin = currententity->origin;
            modelorg = VectorSubtract(r_origin, r_entorigin);
            R_DrawSprite();
        } break;

        case mod_alias: {
            r_entorigin = currententity->origin;
            modelorg = VectorSubtract(r_origin, r_entorigin);

            // see if the bounding box lets us trivially reject, also sets trivial accept status
            if (R_AliasCheckBBox()) {
                int j = R_LightPoint(currententity->origin);

                aLight_t lighting;
                lighting.ambientlight = j;
                lighting.shadelight = j;

                vec3_t lightvec = { .x = -1.0f, .y = 0.0f, .z = 0.0f };
                lighting.plightvec = &lightvec;

                for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
                    if (cl_dlights[lnum].die >= GetClSimTime()) {
                        vec3_t dist = VectorSubtract(currententity->origin, cl_dlights[lnum].origin);
                        float add = cl_dlights[lnum].radius - Length(dist);

                        if (add > 0.0f)
                            lighting.ambientlight += add;
                    }
                }

                // clamp lighting so it doesn't overbright as much
                CLAMP_MORE(&lighting.ambientlight, 128);

                if ((lighting.ambientlight + lighting.shadelight) > 192) // loop?
                    lighting.shadelight = 192 - lighting.ambientlight;

                R_AliasDrawModel(&lighting);
            }

        } break;

        case mod_brush: {/* skipped in R_DrawEntitiesOnList*/ } break;

        default:
            Host_Error(
                "model->type [0x%X] UNKNOWN\n",
                currententity->model->type
            );
            break;
        }
    }
}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel() {
    // FIXME: remove and do real lighting

    if (
        ((!r_drawviewmodel.value) ||
            (r_fov_greater_than_90)
            ) ||
        (cl.items & IT_INVISIBILITY) ||
        (cl.stats[STAT_HEALTH] <= 0)
        )
        return;

    currententity = &cl.viewent;
    if (!currententity->model)
        return;

    r_entorigin = currententity->origin;
    modelorg = VectorSubtract(r_origin, r_entorigin);
    viewlightvec = BS.up;
    VectorInverse(&viewlightvec);

    int j = R_LightPoint(currententity->origin);

    CLAMP_LESS(&j, 24);  // allways give some light on gun

    r_viewlighting.ambientlight = j;
    r_viewlighting.shadelight = j;

    // add dynamic lights
    for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
        dLight_p dl = &cl_dlights[lnum];
        if ((!dl->radius) ||
            (!dl->radius) ||
            (dl->die < GetClSimTime()))
            continue;

        vec3_t dist = VectorSubtract(currententity->origin, dl->origin);
        float add = dl->radius - Length(dist);
        if (add > 0)
            r_viewlighting.ambientlight += add;
    }

    CLAMP_LESS(&r_viewlighting.ambientlight, 128);    // clamp lighting so it doesn't overbright as much

    if ((r_viewlighting.ambientlight + r_viewlighting.shadelight) > 192)
        r_viewlighting.shadelight = 192 - r_viewlighting.ambientlight;

    vec3_t lightvec = { .x = -1.0f, .y = 0.0f, .z = 0.0f };
    r_viewlighting.plightvec = &lightvec;

#ifdef QUAKE2
    cl.light_level = r_viewlighting.ambientlight;
#endif

    R_AliasDrawModel(&r_viewlighting);
}


/*
=============
R_BmodelCheckBBox
=============
*/
extern int* pfrustum_indexes[4];    // TODO: avoid int*
AliasClipFlags_f R_BmodelCheckBBox(Model_p clmodel, BBox_t bb) {
    AliasClipFlags_f clipflags = ALIAS_NON_CLIP;

    if (currententity->angles.pitch ||
        currententity->angles.yaw ||
        currententity->angles.roll
        ) {
        for (int i = 0; i < 4; i++) {
            double d = DotProduct(currententity->origin, view_clipplanes[i].normal);
            d -= view_clipplanes[i].dist;

            if (d <= -clmodel->radius)
                return BMODEL_FULLY_CLIPPED;

            if (d <= clmodel->radius)
                clipflags |= (1 << i);
        }
    }
    else {
        for (int i = 0; i < 4; i++) {
            // generate accept and reject points
            // FIXME: do with fast look-ups or integer tests based on the sign bit of the floating point values

            int* pindex = pfrustum_indexes[i];
            {
                vec3_t rejectpt = {
                    .x = bb.v[pindex[0]],
                    .y = bb.v[pindex[1]],
                    .z = bb.v[pindex[2]]
                };
                double d = DotProduct(rejectpt, view_clipplanes[i].normal) - view_clipplanes[i].dist;
                if (d <= 0.0f)     return BMODEL_FULLY_CLIPPED;
            }
            {
                vec3_t acceptpt = {
                    .x = bb.v[pindex[3 + 0]],
                    .y = bb.v[pindex[3 + 1]],
                    .z = bb.v[pindex[3 + 2]]
                };
                double d = DotProduct(acceptpt, view_clipplanes[i].normal) - view_clipplanes[i].dist;
                if (d <= 0.0f)     clipflags |= (1 << i);
            }
        }
    }

    return clipflags;
}


/*
=============
R_DrawBEntitiesOnList
=============
*/
void R_DrawBEntitiesOnList() {
    if (!r_drawentities.value)  return;

    vec3_t oldorigin = modelorg;
    insubmodel = true;
    r_dlightframecount = r_framecount;

    for (int i = 0; i < cl_numvisedicts; i++) {
        currententity = cl_visedicts[i];

        switch (currententity->model->type) {
        case mod_brush: {
            Model_p clmodel = currententity->model;

            // see if the bounding box lets us trivially reject, also sets trivial accept status
            BBox_t bb = BBoxTranslate(clmodel->BB, currententity->origin);
            AliasClipFlags_f clipflags = R_BmodelCheckBBox(clmodel, bb);

            if (clipflags != BMODEL_FULLY_CLIPPED) {
                r_entorigin = currententity->origin;
                modelorg = VectorSubtract(r_origin, r_entorigin);

                r_pcurrentvertbase = clmodel->vertexes;

                R_RotateBmodel();   // FIXME: stop transforming twice

                // calculate dynamic lighting for bmodel if it's not an instanced model
                if ((r_dlightmap.value) &&
                    (clmodel->firstModelSurface != 0)
                    ) {
                    for (int k = 0; k < MAX_DLIGHTS; k++) {
                        if ((cl_dlights[k].die < GetClSimTime()) ||
                            (!cl_dlights[k].radius)
                            ) {
                            continue;
                        }

                        R_MarkLights(
                            &cl_dlights[k],
                            1 << k,
                            clmodel->nodes + clmodel->hulls[0].firstclipnode
                        );
                    }
                }

                // if the driver wants polygons, deliver those. Z-buffering is on at this point, so no clipping to the world tree is needed, just frustum clipping
                if (r_drawpolys | r_drawculledpolys) {
                    R_ZDrawSubmodelPolys(clmodel);
                }
                else {
                    r_pefragtopnode = NULL;

                    r_entBB = bb;

                    R_SplitEntityOnNode2(cl.worldmodel->nodes);

                    if (r_pefragtopnode) {
                        currententity->topnode = r_pefragtopnode;

                        if (r_pefragtopnode->contents >= CONTENTS_NODE) {   // not a leaf; has to be clipped to the world BSP
                            r_clipflags = clipflags;
                            R_DrawSolidClippedSubmodelPolygons(clmodel);
                        }
                        else {  // falls entirely in one leaf, so we just put all the edges in the edge list and let 1/z sorting handle drawing order
                            R_DrawSubmodelPolygons(clmodel, clipflags);
                        }

                        currententity->topnode = NULL;
                    }
                }

                // put back world rotation and frustum clipping
                // FIXME: R_RotateBmodel should just work off base_vxx
                BS = base_BS;
                modelorg = base_modelorg;
                modelorg = oldorigin;
                R_TransformFrustum();
            }

        } break;

        default:    break;
        }
    }

    insubmodel = false;
}


/*
================
R_EdgeDrawing
================
*/
#include "mem_placement.h"
#if 0
Edge_t ledges[NUMSTACKEDGES + ((CACHE_SIZE - 1) / sizeof(Edge_t)) + 1] PLACE_TO_SDRAM;
Surf_t lsurfs[NUMSTACKSURFACES + ((CACHE_SIZE - 1) / sizeof(Surf_t)) + 1] PLACE_TO_SDRAM;
#else
// TODO: check it and clean --> /* запас +2: выравнивание + место для surfaces-1 */
Edge_t ledges[NUMSTACKEDGES + ((CACHE_SIZE - 1) / sizeof(Edge_t)) + 2] PLACE_TO_SDRAM;
Surf_t lsurfs[NUMSTACKSURFACES + ((CACHE_SIZE - 1) / sizeof(Surf_t)) + 2] PLACE_TO_SDRAM;
#endif

#define ALIGN_PTR(p, a) \
    ((TypeLess_ptr)((((uintptr_t)(p)) + ((a) - 1)) & ~((uintptr_t)((a) - 1))))

void R_EdgeDrawing() {
    r_edges = (auxedges) ?
        auxedges :
        (Edge_p)ALIGN_PTR(&ledges[0], CACHE_SIZE);
    // (((uintptr_t)&ledges[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

    if (r_surfsonstack) {
        /* выравниваем от (lsurfs + 1), чтобы потом surfaces = base - 1 было легально */
        Surf_p base = (Surf_p)ALIGN_PTR(&lsurfs[1], CACHE_SIZE);
        surfaces = base - 1; /* surfaces[1] указывает ровно на base */
        surf_max = &surfaces[r_cnumsurfs];
        /* surface 0 — фиктивный элемент */
        R_SurfacePatch();
    }

    R_BeginEdgeFrame();

    if (r_dspeeds.value) {
        rw_time1 = Host_FloatTime();
    }

    if (r_drawworld.value) {
        R_RenderWorld();
    }

    if (r_drawculledpolys)
        R_ScanEdges();

    // only the world can be drawn back to front with no z reads or compares, just z writes, so have the driver turn z compares on now
    D_TurnZOn();

    if (r_dspeeds.value) {
        rw_time2 = Host_FloatTime();
        db_time1 = rw_time2;
    }

    R_DrawBEntitiesOnList();

    if (r_dspeeds.value) {
        db_time2 = Host_FloatTime();
        se_time1 = db_time2;
    }

    if (!r_dspeeds.value) {
        VID_UnlockBuffer(); S_ExtraUpdate(); VID_LockBuffer(); // don't let sound get messed up if going slow
    }

    if (!(r_drawpolys | r_drawculledpolys))
        R_ScanEdges();
}

/*
    =============
    R_PrintDSpeeds
    =============
*/
void R_PrintDSpeeds() {
    RealTime_t r_time2 = Host_FloatTime();

    RealDt_t dp_time = (dp_time2 - dp_time1) * 1000;
    RealDt_t rw_time = (rw_time2 - rw_time1) * 1000;
    RealDt_t db_time = (db_time2 - db_time1) * 1000;
    RealDt_t se_time = (se_time2 - se_time1) * 1000;
    RealDt_t de_time = (de_time2 - de_time1) * 1000;
    RealDt_t dv_time = (dv_time2 - dv_time1) * 1000;
    RealDt_t ms = /*   */(r_time2 - r_time1) * 1000;

    Con_Printf(
        "%3i %4.1fp %3iw %4.1fb %3is %4.1fe %4.1fv\n",
        (int)ms, dp_time,
        (int)rw_time, db_time,
        (int)se_time, de_time,
        dv_time
    );
}


/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
static  uint8_t _warpbuffer[WARP_WIDTH * WARP_HEIGHT];
void R_RenderView_() {

    r_warpbuffer = _warpbuffer;

    if (r_timegraph.value ||
        r_speeds.value ||
        r_dspeeds.value
        )
        r_time1 = Host_FloatTime();

    R_SetupFrame();

#ifdef PASSAGES
    SetVisibilityByPassages();
#else
    R_MarkLeaves(); // done here so we know if we're in water
#endif

    // make FDIV fast. This reduces timing precision after we've been running for a
    // while, so we don't do it globally.  This also sets chop mode, and we do it
    // here so that setup stuff like the refresh area calculations match what's
    // done in screen.c
    Sys_LowFPPrecision();

    if (!cl_entities[0].model || !cl.worldmodel)
        Host_SysError("R_RenderView: NULL worldmodel");

    if (!r_dspeeds.value) { VID_UnlockBuffer(); S_ExtraUpdate(); VID_LockBuffer(); } // don't let sound get messed up if going slow
    R_EdgeDrawing();
    if (!r_dspeeds.value) { VID_UnlockBuffer(); S_ExtraUpdate(); VID_LockBuffer(); } // don't let sound get messed up if going slow


    if (r_dspeeds.value) {
        se_time2 = Host_FloatTime();
        de_time1 = se_time2;
    }

    R_DrawEntitiesOnList();

    if (r_dspeeds.value) {
        de_time2 = Host_FloatTime();
        dv_time1 = de_time2;
    }

    R_DrawViewModel();

    if (r_dspeeds.value) {
        dv_time2 = Host_FloatTime();
        dp_time1 = Host_FloatTime();
    }

    R_DrawParticles();

    if (r_dspeeds.value)    dp_time2 = Host_FloatTime();
    if (r_dowarp)           D_WarpScreen();

    V_SetContentsColor(r_viewleaf->contents);

    if (r_timegraph.value)  R_TimeGraph();
    if (r_aliasstats.value) R_PrintAliasStats();
    if (r_speeds.value)     R_PrintTimes();
    if (r_dspeeds.value)    R_PrintDSpeeds();

    if (r_reportsurfout.value && r_outofsurfaces)
        Con_Printf("Short %d surfaces\n", r_outofsurfaces);

    if (r_reportedgeout.value && r_outofedges)
        Con_Printf("Short roughly %d edges\n", TWICE(r_outofedges) / 3);

    // back to high floating-point precision
    Sys_HighFPPrecision();
}

void R_RenderView() {
    int dummy;
    int delta = (uint8_p)&dummy - r_stack_start;
    if ((delta < -10000) || (delta > 10000))    Host_SysError("R_RenderView: called without enough stack");
    if (Hunk_LowMark() & 3)                     Host_SysError("Hunk is missaligned");
    if ((uintptr_t)(&dummy) & 3)                Host_SysError("Stack is missaligned");
    if ((uintptr_t)(&r_warpbuffer) & 3)         Host_SysError("Globals are missaligned");

    R_RenderView_();
}


