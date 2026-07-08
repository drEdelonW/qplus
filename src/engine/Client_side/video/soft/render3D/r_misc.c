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
// r_misc.c

#include "r_local.h"
#include "render.h"
#include "screen.h"
#include "host.h"
#include "sbar.h"
#include "q_tools.h"
#include "console.h"
#include "LeafModel.h"


/*
    ===============
    R_CheckVariables
    ===============
*/
void R_CheckVariables() {
    static float _oldBright;

    if (r_fullbright.value != _oldBright) {
        _oldBright = r_fullbright.value;
        D_FlushCaches(); // so all lighting changes
    }
}


/*
    ============
    Show

    Debugging use
    ============
*/
#include "vid.h" // vid.scr.width
void Show() {
    vRect_t vr = {
        // .x      = 0,
        // .y      = 0,
        .width = vid.scr.width,
        .height = vid.scr.height,
        // .pnext  = NULL
    };

    VID_Update(&vr);
}


/*
    ====================
    R_TimeRefresh_f

    For program optimization
    ====================
*/
#define VIEWANGLE_STEPS 128

void R_TimeRefresh_f() {
    int startangle = r_refdef.viewangles.yaw;

    RealTime_t start = Host_FloatTime();
    for (int i = 0; i < VIEWANGLE_STEPS; i++) {
        r_refdef.viewangles.yaw = ((float)i / (float)VIEWANGLE_STEPS) * 360.0;

        VID_LockBuffer();
        R_RenderView();
        VID_UnlockBuffer();

        vRect_t vr = {
            .x = r_refdef.vrect.x,
            .y = r_refdef.vrect.y,
            .width = r_refdef.vrect.width,
            .height = r_refdef.vrect.height,
            // .pnext = NULL,
        };
        VID_Update(&vr);
    }

    RealTime_t stop = Host_FloatTime();
    RealDt_t time = stop - start;
    Con_Printf("%f seconds (%f fps)\n", time, VIEWANGLE_STEPS / time);

    r_refdef.viewangles.yaw = startangle;
}


/*
    ================
    R_LineGraph

    Only called by R_DisplayTime
    ================
*/
#define GRAPH_FG 0xFF  // bright bar color
#define GRAPH_BG 0x30  // background color
void R_LineGraph(int x, int y, int h) {
    // FIXME: should be disabled on no-buffer adapters, or should be in the driver

    x += r_refdef.vrect.x;
    y += r_refdef.vrect.y;

    uint8_p dest = vid.scr.pBuff + (vid.rowbytes * y) + x;

    int s = r_graphheight.value;

    CLAMP(0, &h, s);

    for (int i = 0; i < s; ++i) {
        dest[0] = (i < h) ? GRAPH_FG : GRAPH_BG;

        dest[-vid.rowbytes] = GRAPH_BG;
        dest -= TWICE(vid.rowbytes);
    }
}

/*
    ==============
    R_TimeGraph

    Performance monitoring tool
    ==============
*/
#define MAX_TIMINGS (100)

void R_TimeGraph() {
    static int timex;
    static uint8_t r_timings[MAX_TIMINGS];
    RealTime_t r_time2 = Host_FloatTime();
    int a = (r_time2 - r_time1) / 0.01;
    //a = fabs(mouse_y * 0.05);
    //a = (int)((r_refdef.vieworg[2] + 1024)/1)%(int)r_graphheight.value;
    //a = fabs(velocity[0])/20;
    //a = ((int)fabs(origin[0])/8)%20;
    //a = (cl.idealpitch + 30)/5;
    r_timings[timex] = a;
    a = timex;

    int x =
        r_refdef.vrect.width -
        ((r_refdef.vrect.width <= MAX_TIMINGS) ?
            1 : HALF(r_refdef.vrect.width - MAX_TIMINGS));
    do {
        R_LineGraph(x, r_refdef.vrect.height - 2, r_timings[a]);
        if (x == 0)
            break;  // screen too small to hold entire thing
        x--;
        a--;
        if (a == -1)
            a = MAX_TIMINGS - 1;
    } while (a != timex);

    timex = (timex + 1) % MAX_TIMINGS;
}


/*
    =============
    R_PrintTimes
    =============
*/
void R_PrintTimes() {
    RealTime_t r_time2 = Host_FloatTime();
    RealDt_t ms = 1000 * (r_time2 - r_time1);

    Con_Printf(
        "%5.1f ms %3i/%3i/%3i poly %3i surf\n",
        ms,
        c_faceclip, r_polycount, r_drawnpolycount,
        c_surf
    );
    c_surf = 0;
}



/*
    =============
    R_PrintAliasStats
    =============
*/
void R_PrintAliasStats() {
    Con_Printf(
        "%3i polygon model drawn\n",
        r_amodels_drawn
    );
}


void WarpPalette() {
    qPal_t newpalette;
    qRgb24 basecolor = {
        .r = 130,
        .g = 80,
        .b = 50
    };

    // pull the colors halfway to bright brown
    for (int idx = 0; idx < InksNum; idx++) {
        for (int c = 0; c < RGB_DIM; c++) {
            newpalette.ink[idx].ch[c] = HALF(host_basepal->ink[idx].ch[c] + basecolor.ch[c]);
        }
    }

    VID_ShiftPalette(&newpalette);
}


/*
    ===================
    R_TransformFrustum
    ===================
*/
void R_TransformFrustum() {

    for (int i = 0; i < 4; i++) {
        vec3_t v = {        // Map screen-space normal components to camera basis:
            .x = screenedge[i].normal.z,
            .y = -screenedge[i].normal.x,
            .z = screenedge[i].normal.y
        };

        vec3_t v2 = VectorMA(VectorMA(
            VectorScale(BS.forward, v.x),
            v.y, BS.right),
            v.z, BS.up
        );

        view_clipplanes[i].normal = v2;

        view_clipplanes[i].dist = DotProduct(modelorg, v2);
    }
}


#if !id386

/*
    ================
    TransformVector
    ================
*/
vec3_t/*void*/  TransformVector(vec3_t in /*, vec3_p out */) {
    vec3_t out = (vec3_t){

        .x = DotProduct(in, BS.right),
        .y = DotProduct(in, BS.up),
        .z = DotProduct(in, BS.forward)
    };
    return out;
}

#endif

#if 0  // not used
/*
    ================
    R_TransformPlane
    ================
*/
void R_TransformPlane(mPlane_p p, vec3_p normal, float_p dist) {
    float d = DotProduct(r_origin, p->normal);
    *dist = p->dist - d;
    // TODO: when we have rotating entities, this will need to use the view matrix
    *normal = TransformVector(p->normal);
}
#endif

/*
    ===============
    R_SetUpFrustumIndexes
    ===============
*/
static int _r_frustum_indexes[4 * 6];
int* pfrustum_indexes[4];

void R_SetUpFrustumIndexes() {
    int* pindex = _r_frustum_indexes;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < VECT_DIM; j++) {
            if (view_clipplanes[i].normal.v[j] < 0.0f) {
                pindex[j] = j;
                pindex[j + 3] = j + 3;
            }
            else {
                pindex[j] = j + 3;
                pindex[j + 3] = j;
            }
        }

        // FIXME: do just once at start
        pfrustum_indexes[i] = pindex;
        pindex += 6;
    }
}


/*
    ===============
    R_SetupFrame
    ===============
*/
void R_SetupFrame() {
    // don't allow cheats in multiplayer
    if (cl.maxclients > 1) {
        Cvar_Set("r_draworder", "0");
        Cvar_Set("r_fullbright", "0");
        Cvar_Set("r_ambient", "0");
        Cvar_Set("r_drawflat", "0");
    }

    if (r_numsurfs.value) {
        CLAMP_MIN(&r_maxsurfsseen, (surface_p - surfaces));

        Con_Printf("Used %d of %d surfs; %d max\n",
            surface_p - surfaces,
            surf_max - surfaces,
            r_maxsurfsseen
        );
    }

    if (r_numedges.value) {
        int edgecount = edge_p - r_edges;

        CLAMP_MIN(&r_maxedgesseen, edgecount);

        Con_Printf("Used %d of %d edges; %d max\n",
            edgecount, r_numallocatededges, r_maxedgesseen
        );
    }

    r_refdef.ambientlight = r_ambient.value;

    CLAMP_MIN(&r_refdef.ambientlight, 0);

    if (!Host_IsServerActive())
        r_draworder.value = 0; // don't let cheaters look behind walls

    R_CheckVariables();

    R_AnimateLight();

    r_framecount++;

    numbtofpolys = 0;

    // debugging
#if 0
    r_refdef.vieworg = { .x = 80.0f, .y = 64.0f, .z = 40.0f };
    r_refdef.viewangles = { .pitch = 0.0f, .yaw = 46.763641357, .roll = 0.0f };
#endif

    // build the transformation matrix for the given view angles
    modelorg = r_refdef.vieworg;
    r_origin = r_refdef.vieworg;

    BS = GetBasis(r_refdef.viewangles);
    // current viewleaf
    r_oldviewleaf = r_viewleaf;
    r_viewleaf = Mod_PointInLeaf(r_origin, cl.worldmodel);

    r_dowarpold = r_dowarp;
    r_dowarp = r_waterwarp.value && (r_viewleaf->contents <= CONTENTS_WATER);

    if ((r_dowarp != r_dowarpold) ||
        r_viewchanged ||
        lcd_x.value
        ) {
        if (r_dowarp) {
            if ((vid.scr.width <= vid.maxwarp.width) &&
                (vid.scr.height <= vid.maxwarp.height)
                ) {
                vRect_t vrect = {
                    .width = vid.scr.width,
                    .height = vid.scr.height
                };

                R_ViewChanged(&vrect, sb_lines, scr.aspect);
            }
            else {
                float w = vid.scr.width;
                float h = vid.scr.height;

                if (w > vid.maxwarp.width) {
                    h *= (float)vid.maxwarp.width / w;
                    w = vid.maxwarp.width;
                }

                if (h > vid.maxwarp.height) {
                    h = vid.maxwarp.height;
                    w *= (float)vid.maxwarp.height / h;
                }

                vRect_t vrect = {
                    .width = (int)w,
                    .height = (int)h
                };

                R_ViewChanged(&vrect,
                    (int)((float)sb_lines * (h / (float)vid.scr.height)),
                    scr.aspect * (h / w) *
                    ((float)vid.scr.width / (float)vid.scr.height));
            }
        }
        else {
            vRect_t vrect = {
                .width = vid.scr.width,
                .height = vid.scr.height
            };

            R_ViewChanged(&vrect, sb_lines, scr.aspect);
        }

        r_viewchanged = false;
    }

    // start off with just the four screen edge clip planes
    R_TransformFrustum();

    // save base values

    base_BS = BS;
    base_modelorg = modelorg;

    R_SetSkyFrame();

    R_SetUpFrustumIndexes();

    r_cache_thrash = false;

    // clear frame counts
    c_faceclip = 0;
    d_spanpixcount = 0;
    r_polycount = 0;
    r_drawnpolycount = 0;
    r_wholepolycount = 0;
    r_amodels_drawn = 0;
    r_outofsurfaces = 0;
    r_outofedges = 0;

    D_SetupFrame();
}

