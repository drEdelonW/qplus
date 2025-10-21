#pragma once
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
#ifndef GLQUAKE

#include "cvar_q1.h"
#include "d_iface.h"
// r_shared.h: general refresh-related stuff shared between the refresh and the
// driver

// FIXME: clean up and move into d_iface.h


#define MAXVERTS 16     // max points in a surface polygon
#define MAXWORKINGVERTS (MAXVERTS+4) // max points in an intermediate
                                        //  polygon (while processing)
// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define MAXHEIGHT  1024
#define MAXWIDTH  1280
#define MAXDIMENSION ((MAXHEIGHT > MAXWIDTH) ? MAXHEIGHT : MAXWIDTH)

#define SIN_BUFFER_SIZE (MAXDIMENSION + CYCLE)

#define INFINITE_DISTANCE 0x10000  // distance that's always guaranteed to
                                        //  be farther away than anything in
                                        //  the scene

//===================================================================

extern void R_DrawLine(PolyVert_p polyvert0, PolyVert_p polyvert1);

extern int      cachewidth;
extern pixel_p  cacheblock;
extern int      screenwidth;
extern float    pixelAspect;
extern int      r_drawnpolycount;
extern int      sintable[SIN_BUFFER_SIZE];
extern int      intsintable[SIN_BUFFER_SIZE];
extern vec3_t   vup, base_vup;
extern vec3_t   vpn, base_vpn;
extern vec3_t   vright, base_vright;
extern r_Entity_p   currententity;

#define NUMSTACKEDGES       (2400)
#define MINEDGES            NUMSTACKEDGES
#define NUMSTACKSURFACES    (800)
#define MINSURFACES         NUMSTACKSURFACES
#define MAXSPANS            (3000)

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct eSpan_s eSpan_t;
typedef eSpan_t* eSpan_p;
struct eSpan_s {
    int     u;
    int     v;
    int     count;
    eSpan_p pnext;
};

typedef enum {
    invSpan = -1,   // = in inverted span (end before start)
    notInSpan = 0,  // = not in span
    inSpan = 1,     // = in span
} SnapState_e;
// FIXME: compress, make a union if that will help
// insubmodel is only 1, flags is fewer than 32, spanstate could be a byte
typedef struct Surf_s Surf_t;
typedef Surf_t* Surf_p;
struct Surf_s {
    Surf_p  next;       // active surface stack in r_edge.c
    Surf_p  prev;       // used in r_edge.c for active surf stack
    eSpan_p spans;      // pointer to linked list of spans to draw
    int     key;        // sorting key (BSP order)
    int     last_u;     // set during tracing
    SnapState_e spanstate;
    // int     spanstate;  // 0 = not in span
    // 1 = in span
    // -1 = in inverted span (end before
    //  start)
    int     flags;      // currentface flags
    TypeLess_ptr    data;   // associated data like mSurface_t
    r_Entity_p  entity;
    float   nearzi;     // nearest 1/z on surface, for mipmapping
    qboolean    insubmodel;
    float   d_ziorigin, d_zistepu, d_zistepv;

    int   pad[2];    // to 64 bytes
};

extern Surf_p surfaces, surface_p, surf_max;

// surfaces are generated in back to front order by the bsp, so if a surf
// pointer is greater than another one, it should be drawn in front
// surfaces[1] is the background, and is used as the active surface stack.
// surfaces[0] is a dummy, because index 0 is used to indicate no surface
//  attached to an Edge_t

//===================================================================

extern vec3_t sxformaxis[4]; // s axis transformed into viewspace
extern vec3_t txformaxis[4]; // t axis transformed into viewspac

extern vec3_t modelorg, base_modelorg;

extern float xcenter, ycenter;
extern float xscale, yscale;
extern float xscaleinv, yscaleinv;
extern float xscaleshrink, yscaleshrink;

extern int d_lightstylevalue[256]; // 8.8 frac of base light value

extern void TransformVector(vec3_t in, vec3_t out);
extern void SetUpForLineScan(fixed8_t startvertu, fixed8_t startvertv,
    fixed8_t endvertu, fixed8_t endvertv);

extern int r_skymade;
extern void R_MakeSky();

extern int ubasestep, errorterm, erroradjustup, erroradjustdown;


// !!! if this is changed, it must be changed in asm_draw.h too !!!
struct Edge_s;
typedef struct Edge_s Edge_t;
typedef Edge_t* Edge_p;
struct Edge_s {
    fixed16_t   u;
    fixed16_t   u_step;
    Edge_p      prev;
    Edge_p      next;
    uint16_t    surfs[2];
    Edge_p      nextremove;
    float       nearzi;
    mEdge_p     owner;
};


#endif // GLQUAKE
