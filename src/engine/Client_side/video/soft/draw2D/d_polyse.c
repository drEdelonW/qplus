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
// d_polyset.c: routines for drawing sets of polygons sharing the same
// texture (used for Alias models)

#include "r_local.h"
#include "d_local.h"

// TODO: put in span spilling to shrink list size
// !!! if this is changed, it must be changed in d_polysa.s too !!!
#define DPS_MAXSPANS   MAXHEIGHT+1
                                    // 1 extra for spanpackage that marks end

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
    pixel_p     pdest;
    int16_p     pz;
    int8_p      ptex;

    int         count;
#if 1
    fixed16_t   sfrac;
    fixed16_t   tfrac;
    fixed16_t   light;
    fixed16_t   zi;
#else
    // TODO: make VertexAttr-like structure
#endif
} SpanPackage_t;
typedef SpanPackage_t* SpanPackage_p;

int errorterm;
int erroradjustup;
int erroradjustdown;


typedef struct {
    int  isflattop;     // TODO: not used
    int  numLeftEdges;
    VertAttr_p pLeftEdgeVert[3];
    int  numRightEdges;
    VertAttr_p pRightEdgeVert[3];
} EdgeTable_t;
typedef EdgeTable_t* EdgeTable_p;

static VertAttr_t _p[3];

pixel_p acolormap;
pixel_p d_pcolormap;

int   d_aflatcolor;
int   d_xdenom;

EdgeTable_p pedgetable;

EdgeTable_t edgetables[12] = {
    {0, 1, {&_p[0], &_p[2], NULL  },    2, {&_p[0], &_p[1], &_p[2]}},
    {0, 2, {&_p[1], &_p[0], &_p[2]},    1, {&_p[1], &_p[2], NULL  }},
    {1, 1, {&_p[0], &_p[2], NULL  },    1, {&_p[1], &_p[2], NULL  }},
    {0, 1, {&_p[1], &_p[0], NULL  },    2, {&_p[1], &_p[2], &_p[0]}},
    {0, 2, {&_p[0], &_p[2], &_p[1]},    1, {&_p[0], &_p[1], NULL  }},
    {0, 1, {&_p[2], &_p[1], NULL  },    1, {&_p[2], &_p[0], NULL  }},
    {0, 1, {&_p[2], &_p[1], NULL  },    2, {&_p[2], &_p[0], &_p[1]}},
    {0, 2, {&_p[2], &_p[1], &_p[0]},    1, {&_p[2], &_p[0], NULL  }},
    {0, 1, {&_p[1], &_p[0], NULL  },    1, {&_p[1], &_p[2], NULL  }},
    {1, 1, {&_p[2], &_p[1], NULL  },    1, {&_p[0], &_p[1], NULL  }},
    {1, 1, {&_p[1], &_p[0], NULL  },    1, {&_p[2], &_p[0], NULL  }},
    {0, 1, {&_p[0], &_p[2], NULL  },    1, {&_p[0], &_p[1], NULL  }},
};

static inline int getOrder() {
    int edgeTableIdx = 0; // assume the vertices are already in top to bottom order
    // determine which edges are right & left, and the order in which to rasterize them
    if (_p[0].y >= _p[1].y) {
        if (_p[0].y == _p[1].y)         return (_p[0].y < _p[2].y) ? 2 : 5;
        else                    edgeTableIdx = 1;
    }
    /* */if (_p[0].y == _p[2].y)        return (edgeTableIdx) ? 8 : 9;
    else if (_p[1].y == _p[2].y)        return (edgeTableIdx) ? 10 : 11;
    if (_p[0].y > _p[2].y)      edgeTableIdx += 2;
    if (_p[1].y > _p[2].y)      edgeTableIdx += 4;

    return edgeTableIdx;
}

/*
================
D_PolysetSetEdgeTable
================
*/
void D_PolysetSetEdgeTable() {
    int edgetableindex = 0; // assume the vertices are already in top to bottom order

    //
    // determine which edges are right & left, and the order in which
    // to rasterize them
    //
    if (_p[0].y >= _p[1].y) {
        if (_p[0].y == _p[1].y) {
            pedgetable = (_p[0].y < _p[2].y) ? &edgetables[2] : &edgetables[5];
            return;
        }
        else    edgetableindex = 1;

    }

    if (_p[0].y == _p[2].y) {
        pedgetable = (edgetableindex) ? &edgetables[8] : &edgetables[9];
        return;
    }
    else if (_p[1].y == _p[2].y) {
        pedgetable = (edgetableindex) ? &edgetables[10] : &edgetables[11];
        return;
    }

    if (_p[0].y > _p[2].y)      edgetableindex += 2;
    if (_p[1].y > _p[2].y)      edgetableindex += 4;

    pedgetable = &edgetables[edgetableindex];
}

// FIXME: some of these can become statics
int a_sstepxfrac, a_tstepxfrac;

int r_sstepx, r_tstepx, r_lstepx, r_zistepx;    // s t l zi
int r_sstepy, r_tstepy, r_lstepy, r_zistepy;    // s t l zi

int a_ststepxwhole;

SpanPackage_p a_spans;
SpanPackage_p d_pedgespanpackage;
static int    _yStart;

SpanPackage_t d_snap;
SpanPackage_t d_basestep;
SpanPackage_t d_extrastep;

typedef struct {
    int  quotient;
    int  remainder;
} adivtab_t;
typedef adivtab_t* adivtab_p;

static adivtab_t _aDivTab[32 * 32] = {
#include "adivtab.h"
};

uint8_p skintable[MAX_LBM_HEIGHT];
uint8_p skinstart;
int  skinwidth;

void D_PolysetDrawSpans8(SpanPackage_p pspanpackage);
void D_PolysetCalcGradients(int skinwidth);
void D_DrawSubdiv();
void D_DrawNonSubdiv();
void D_PolysetRecursiveTriangle(VertAttr_p p1, VertAttr_p p2, VertAttr_p p3);
// void D_PolysetSetEdgeTable();
void D_RasterizeAliasPolySmooth();
void D_PolysetScanLeftEdge(int height);

#if !id386

/*
================
D_PolysetDraw
================
*/
void D_PolysetDraw() {
    SpanPackage_t spans[
        DPS_MAXSPANS + 1 +
            ((CACHE_SIZE - 1) / sizeof(SpanPackage_t)) + 1
    ];
    // one extra because of cache line pretouching

    a_spans = (SpanPackage_p)
        (((uintptr_t)&spans[0] + CACHE_SIZE - 1) & ~(uintptr_t)(CACHE_SIZE - 1));

    if (r_affinetridesc.drawtype)   D_DrawSubdiv();
    else                            D_DrawNonSubdiv();

}


/*
================
D_PolysetDrawFinalVerts
================
*/
void D_PolysetDrawFinalVerts(FinalVert_p fv, int numverts) {
    for (int i = 0; i < numverts; i++, fv++) {
        // valid triangle coordinates for filling can include the bottom and right clip edges, due to the fill rule; these shouldn't be drawn
        if ((fv->vAttr.x < r_refdef.vrectright) &&
            (fv->vAttr.y < r_refdef.vrectbottom)
            ) {
            int z = FIXED16_TO_INT(fv->vAttr.zi);
            int16_p zbuf = zspantable[fv->vAttr.y] + fv->vAttr.x;
            if (z >= *zbuf) {
                *zbuf = z;
                d_viewbuffer[d_scantable[fv->vAttr.y] + fv->vAttr.x] =
                    acolormap[
                        skintable[FIXED16_TO_INT(fv->vAttr.t)][FIXED16_TO_INT(fv->vAttr.s)] +
                            (fv->vAttr.light & 0xFF00)
                    ];
            }
        }
    }
}


/*
================
D_DrawSubdiv
================
*/
void D_DrawSubdiv() {
    FinalVert_p pfv = r_affinetridesc.pfinalverts;
    mTriangle_p ptri = r_affinetridesc.ptriangles;
    int lnumtriangles = r_affinetridesc.numtriangles;

    for (int i = 0; i < lnumtriangles; i++) {
        FinalVert_p index0 = pfv + ptri[i].vertindex[0];
        FinalVert_p index1 = pfv + ptri[i].vertindex[1];
        FinalVert_p index2 = pfv + ptri[i].vertindex[2];

        if (((index0->vAttr.y - index1->vAttr.y) * (index0->vAttr.x - index2->vAttr.x) -
            (index0->vAttr.x - index1->vAttr.x) * (index0->vAttr.y - index2->vAttr.y)) >= 0) {
            continue;
        }

        d_pcolormap = &acolormap[index0->vAttr.light & 0xFF00];

        if (ptri[i].facesfront) {
            D_PolysetRecursiveTriangle(&index0->vAttr, &index1->vAttr, &index2->vAttr);
        }
        else {
            int s0 = index0->vAttr.s;
            int s1 = index1->vAttr.s;
            int s2 = index2->vAttr.s;

            if (index0->flags & ALIAS_ONSEAM)   index0->vAttr.s += r_affinetridesc.seamfixupX16;
            if (index1->flags & ALIAS_ONSEAM)   index1->vAttr.s += r_affinetridesc.seamfixupX16;
            if (index2->flags & ALIAS_ONSEAM)   index2->vAttr.s += r_affinetridesc.seamfixupX16;

            D_PolysetRecursiveTriangle(&index0->vAttr, &index1->vAttr, &index2->vAttr);

            index0->vAttr.s = s0;
            index1->vAttr.s = s1;
            index2->vAttr.s = s2;
        }
    }
}


/*
================
D_DrawNonSubdiv
================
*/
void D_DrawNonSubdiv() {
    FinalVert_p pfv = r_affinetridesc.pfinalverts;
    mTriangle_p ptri = r_affinetridesc.ptriangles;
    int lnumtriangles = r_affinetridesc.numtriangles;

    for (int i = 0; i < lnumtriangles; i++, ptri++) {
        FinalVert_p index0 = pfv + ptri->vertindex[0];
        FinalVert_p index1 = pfv + ptri->vertindex[1];
        FinalVert_p index2 = pfv + ptri->vertindex[2];

        d_xdenom =
            (index0->vAttr.y - index1->vAttr.y) * (index0->vAttr.x - index2->vAttr.x) -
            (index0->vAttr.x - index1->vAttr.x) * (index0->vAttr.y - index2->vAttr.y);

        if (d_xdenom >= 0) { continue; }

        _p[0] = index0->vAttr;
        _p[1] = index1->vAttr;
        _p[2] = index2->vAttr;

        if (!ptri->facesfront) {
            if (index0->flags & ALIAS_ONSEAM)   _p[0].s += r_affinetridesc.seamfixupX16;
            if (index1->flags & ALIAS_ONSEAM)   _p[1].s += r_affinetridesc.seamfixupX16;
            if (index2->flags & ALIAS_ONSEAM)   _p[2].s += r_affinetridesc.seamfixupX16;
        }

#if 0
        D_PolysetSetEdgeTable();
#else
        pedgetable = &edgetables[getOrder()];
#endif
        D_RasterizeAliasPolySmooth();
    }
}


/*
================
D_PolysetRecursiveTriangle
================
*/
void D_PolysetRecursiveTriangle(VertAttr_p lp1, VertAttr_p lp2, VertAttr_p lp3) {
    int d;
    if (
        (
            ((d = lp2->x - lp1->x) < -1) ||
            (d > 1)) ||
        (
            ((d = lp2->y - lp1->y) < -1) ||
            (d > 1))
        ) {
        /*  no rotation needed. do nothing */
    }
    else  if (
        (
            ((d = lp3->x - lp2->x) < -1) ||
            (d > 1)) ||
        (
            ((d = lp3->y - lp2->y) < -1) ||
            (d > 1))
        ) {
            {   // rotate
                VertAttr_p temp = lp1;
                lp1 = lp2;
                lp2 = lp3;
                lp3 = temp;
            }
    }
    else if (
        (
            ((d = lp1->x - lp3->x) < -1) ||
            (d > 1)) ||
        (
            ((d = lp1->y - lp3->y) < -1) ||
            (d > 1))
        ) {
            {   // rotate opposite
                VertAttr_p temp = lp1;
                lp1 = lp3;
                lp3 = lp2;
                lp2 = temp;
            }
    }
    else {
        return;   // entire tri is filled
    }

    // split this edge
    VertAttr_t  _new = {
        .x = FIXED_MID(lp1->x, lp2->x),
        .y = FIXED_MID(lp1->y, lp2->y),
        .s = FIXED_MID(lp1->s, lp2->s),
        .t = FIXED_MID(lp1->t, lp2->t),
        /* .light — skipped */
        .zi = FIXED_MID(lp1->zi, lp2->zi),
    };
    if ((_new.y < 0) ||
        (_new.x < 0)
        ) {
        return;
    }
    // draw the point if splitting a leading edge
    if (
        !(
            (lp2->y > lp1->y) ||
            (
                (lp2->y == lp1->y) &&
                (lp2->x < lp1->x))
            )
        ) {

        int z = FIXED16_TO_INT(_new.zi);
        if (_new.y >= MAXHEIGHT) {
            printf("CRAP! D_PolysetRecursiveTriangle 0x%X %i\n", _new.y, _new.y);
            return;
        }

        int16_p zbuf = zspantable[_new.y] + _new.x;
        if (z >= *zbuf) {
            *zbuf = z;
            d_viewbuffer[d_scantable[_new.y] + _new.x] =
                d_pcolormap[skintable[FIXED16_TO_INT(_new.t)][FIXED16_TO_INT(_new.s)]];
        }
    }
    // recursively continue
    D_PolysetRecursiveTriangle(lp3, lp1, &_new);
    D_PolysetRecursiveTriangle(lp3, &_new, lp2);
}

#endif // !id386


/*
================
D_PolysetUpdateTables
================
*/
void D_PolysetUpdateTables() {
    if ((r_affinetridesc.skinwidth != skinwidth) ||
        (r_affinetridesc.pskin != skinstart)
        ) {
        skinwidth = r_affinetridesc.skinwidth;
        skinstart = r_affinetridesc.pskin;
        uint8_p s = skinstart;
        for (int i = 0; i < MAX_LBM_HEIGHT; i++, s += skinwidth)
            skintable[i] = s;
    }
}


#if !id386

/*
===================
D_PolysetScanLeftEdge
====================
*/
void D_PolysetScanLeftEdge(int height) {
    do {
        *d_pedgespanpackage = d_snap;
        d_pedgespanpackage++;

        errorterm += erroradjustup;
        if (errorterm >= 0) {
            d_snap.pdest += (ptrdiff_t)d_extrastep.pdest;
            d_snap.pz += (ptrdiff_t)d_extrastep.pz;
            d_snap.count += d_extrastep.count;
            d_snap.ptex += (ptrdiff_t)d_extrastep.ptex;
            d_snap.sfrac += d_extrastep.sfrac;
            d_snap.ptex += FIXED16_TO_INT(d_snap.sfrac);
            d_snap.sfrac &= 0xFFFF;
            d_snap.tfrac += d_extrastep.tfrac;
            if (d_snap.tfrac & 0x10000) {
                d_snap.ptex += r_affinetridesc.skinwidth;
                d_snap.tfrac &= 0xFFFF;
            }
            d_snap.light += d_extrastep.light;
            d_snap.zi += d_extrastep.zi;

            errorterm -= erroradjustdown;
        }
        else {
            d_snap.pdest += (ptrdiff_t)d_basestep.pdest;
            d_snap.pz += (ptrdiff_t)d_basestep.pz;
            d_snap.count += d_basestep.count;
            d_snap.ptex += (ptrdiff_t)d_basestep.ptex;
            d_snap.sfrac += d_basestep.sfrac;
            d_snap.ptex += FIXED16_TO_INT(d_snap.sfrac);
            d_snap.sfrac &= 0xFFFF;
            d_snap.tfrac += d_basestep.tfrac;
            if (d_snap.tfrac & 0x10000) {
                d_snap.ptex += r_affinetridesc.skinwidth;
                d_snap.tfrac &= 0xFFFF;
            }
            d_snap.light += d_basestep.light;
            d_snap.zi += d_basestep.zi;
        }
    } while (--height);
}

#endif // !id386


/*
===================
D_PolysetSetUpForLineScan
====================
*/
void D_PolysetSetUpForLineScan(
    int startvertu,
    int startvertv,
    int endvertu,
    int endvertv
) {
    // TODO: implement x86 version

    errorterm = -1;

    int tm = endvertu - startvertu;
    int tn = endvertv - startvertv;

    if (((tm <= 16) && (tm >= -15)) &&
        ((tn <= 16) && (tn >= -15))
        ) {
        adivtab_p ptemp = &_aDivTab[(MUL32(tm + 15)) + (tn + 15)];
        d_basestep.count = ptemp->quotient;
        erroradjustup = ptemp->remainder;
        erroradjustdown = tn;
    }
    else {
        double dm = (double)tm;
        double dn = (double)tn;

        FloorDivMod(dm, dn, &d_basestep.count, &erroradjustup);

        erroradjustdown = dn;
    }
}


#if !id386

/*
================
D_PolysetCalcGradients
================
*/
void D_PolysetCalcGradients(int skinwidth) {
    float p00_minus_p20 = _p[0].x - _p[2].x;
    float p01_minus_p21 = _p[0].y - _p[2].y;
    float p10_minus_p20 = _p[1].x - _p[2].x;
    float p11_minus_p21 = _p[1].y - _p[2].y;

    float xstepdenominv = 1.0 / (float)d_xdenom;

    float ystepdenominv = -xstepdenominv;

    // ceil() for light so positive steps are exaggerated, negative steps
    // diminished,  pushing us away from underflow toward overflow. Underflow is
    // very visible, overflow is very unlikely, because of ambient lighting
    {
        float t0 = _p[0].light - _p[2].light;
        float t1 = _p[1].light - _p[2].light;
        r_lstepx = (int)ceil((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
        r_lstepy = (int)ceil((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);
    }
    {
        float t0 = _p[0].s - _p[2].s;
        float t1 = _p[1].s - _p[2].s;
        r_sstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
        r_sstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);
    }
    {
        float t0 = _p[0].t - _p[2].t;
        float t1 = _p[1].t - _p[2].t;
        r_tstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
        r_tstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);
    }
    {
        float t0 = _p[0].zi - _p[2].zi;
        float t1 = _p[1].zi - _p[2].zi;
        r_zistepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
        r_zistepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);
    }
#if id386
    a_sstepxfrac = INT_TO_FIXED16(r_sstepx);
    a_tstepxfrac = INT_TO_FIXED16(r_tstepx);
#else
    a_sstepxfrac = FIXED16_FRAC(r_sstepx);
    a_tstepxfrac = FIXED16_FRAC(r_tstepx);
#endif

    a_ststepxwhole = skinwidth * FIXED16_TO_INT(r_tstepx) + FIXED16_TO_INT(r_sstepx);
}

#endif // !id386


#if 0
uint8_t gelmap[256];
void InitGel(uint8_p palette) {
    for (int i = 0; i < 256; i++) {
        //  r = (palette[i*3]>>4);
        int r = (palette[i * 3] + palette[i * 3 + 1] + palette[i * 3 + 2]) / (16 * 3);
        gelmap[i] = /* 64 */ 0 + r;
    }
}
#endif


#if !id386

/*
================
D_PolysetDrawSpans8
================
*/
void D_PolysetDrawSpans8(SpanPackage_p pspanpackage) {
    do {
        int lcount = d_snap.count - pspanpackage->count;

        errorterm += erroradjustup;
        if (errorterm >= 0) {
            d_snap.count += d_extrastep.count;
            errorterm -= erroradjustdown;
        }
        else d_snap.count += d_basestep.count;

        if (lcount) {
            SpanPackage_t l = *pspanpackage;

            do {
                if (FIXED16_TO_INT(l.zi) >= *l.pz) {
                    *l.pdest = acolormap[*l.ptex + (l.light & 0xFF00)];
                    // gel mapping     *lpdest = gelmap[*lpdest];
                    *l.pz = FIXED16_TO_INT(l.zi);
                }
                l.pdest++;
                l.zi += r_zistepx;
                l.pz++;
                l.light += r_lstepx;
                l.ptex += a_ststepxwhole;
                l.sfrac += a_sstepxfrac;
                l.ptex += FIXED16_TO_INT(l.sfrac);
                l.sfrac &= 0xFFFF;
                l.tfrac += a_tstepxfrac;
                if (l.tfrac & 0x10000) {
                    l.ptex += r_affinetridesc.skinwidth;
                    l.tfrac &= 0xFFFF;
                }
            } while (--lcount);
        }

        pspanpackage++;
    } while (pspanpackage->count != -999999);
}
#endif // !id386


/*
================
D_PolysetFillSpans8
================
*/
void D_PolysetFillSpans8(SpanPackage_p pspanpackage) {
    // FIXME: do z buffering
    int color = d_aflatcolor++;

    while (1) {
        int lcount = pspanpackage->count;

        if (lcount == -1)   return;
        if (lcount) {
            uint8_p lpdest = pspanpackage->pdest;

            do {
                *lpdest++ = color;
            } while (--lcount);
        }

        pspanpackage++;
    }
}

/*
================
D_RasterizeAliasPolySmooth
================
*/
void D_RasterizeAliasPolySmooth() {
    VertAttr_p plefttop = pedgetable->pLeftEdgeVert[0];
    VertAttr_p prighttop = pedgetable->pRightEdgeVert[0];
    VertAttr_p pleftbottom = pedgetable->pLeftEdgeVert[1];
    VertAttr_p prightbottom = pedgetable->pRightEdgeVert[1];

    int initialleftheight = pleftbottom->y - plefttop->y;
    int initialrightheight = prightbottom->y - prighttop->y;

    // set the s, t, and light gradients, which are consistent across the triangle
    // because being a triangle, things are affine
    D_PolysetCalcGradients(r_affinetridesc.skinwidth);

    // rasterize the polygon
    // scan out the top (and possibly only) part of the left edge
    d_pedgespanpackage = a_spans;

    _yStart = plefttop->y;
    d_snap.count = plefttop->x - prighttop->x;

    d_snap.ptex = (r_affinetridesc.pskin +
        FIXED16_TO_INT(plefttop->s) +
        FIXED16_TO_INT(plefttop->t) * r_affinetridesc.skinwidth
        );
#if id386
    d_snap.sfrac = FIXED16_FRAC(plefttop->s) << 16;
    d_snap.tfrac = FIXED16_FRAC(plefttop->t) << 16;
#else
    d_snap.sfrac = FIXED16_FRAC(plefttop->s);
    d_snap.tfrac = FIXED16_FRAC(plefttop->t);
#endif
    d_snap.light = plefttop->light;
    d_snap.zi = plefttop->zi;

    d_snap.pdest = (d_viewbuffer + (_yStart * screenwidth) + plefttop->x);
    d_snap.pz = (d_pzbuffer + (_yStart * d_zwidth) + plefttop->x);

    if (initialleftheight == 1) {
        *d_pedgespanpackage = d_snap;
        d_pedgespanpackage++;
    }
    else {
        D_PolysetSetUpForLineScan(
            plefttop->x, plefttop->y,
            pleftbottom->x, pleftbottom->y
        );

#if id386
        d_pzbasestep = (int16_p)(d_zwidth + d_basestep.count) << 1;
        d_pzextrastep = d_pzbasestep + 2;
#else
        d_extrastep.count = d_basestep.count + 1;

        d_basestep.pz = (int16_p)(intptr_t)(d_zwidth + d_basestep.count);
        d_extrastep.pz = (int16_p)(intptr_t)(d_zwidth + d_extrastep.count);
#endif

        d_basestep.pdest = (TypeLess_ptr)(intptr_t)(screenwidth + d_basestep.count);
        d_extrastep.pdest = (TypeLess_ptr)(intptr_t)(screenwidth + d_extrastep.count);

        // TODO: can reuse partial expressions here

        // for negative steps in x along left edge, bias toward overflow rather than
        // underflow (sort of turning the floor() we did in the gradient calcs into
        // ceil(), but plus a little bit)
        int working_lstepx = (d_basestep.count < 0) ?
            (r_lstepx - 1) : r_lstepx;
        d_basestep.ptex = (TypeLess_ptr)(
            FIXED16_TO_INT(r_sstepy + r_sstepx * d_basestep.count) +
            FIXED16_TO_INT(r_tstepy + r_tstepx * d_basestep.count) * r_affinetridesc.skinwidth
            );
#if id386
        d_basestep.sfrac = (r_sstepy + r_sstepx * d_basestep.count) << 16;
        d_basestep.tfrac = (r_tstepy + r_tstepx * d_basestep.count) << 16;
#else
        d_basestep.sfrac = FIXED16_FRAC(r_sstepy + r_sstepx * d_basestep.count);
        d_basestep.tfrac = FIXED16_FRAC(r_tstepy + r_tstepx * d_basestep.count);
#endif
        d_basestep.light = r_lstepy + working_lstepx * d_basestep.count;
        d_basestep.zi = r_zistepy + r_zistepx * d_basestep.count;

        d_extrastep.ptex = (TypeLess_ptr)(
            FIXED16_TO_INT(r_sstepy + r_sstepx * d_extrastep.count) +
            FIXED16_TO_INT(r_tstepy + r_tstepx * d_extrastep.count) * r_affinetridesc.skinwidth
            );
#if id386
        d_extrastep.sfrac = (r_sstepy + r_sstepx * d_extrastep.count) << 16;
        d_extrastep.tfrac = (r_tstepy + r_tstepx * d_extrastep.count) << 16;
#else
        d_extrastep.sfrac = FIXED16_FRAC(r_sstepy + r_sstepx * d_extrastep.count);
        d_extrastep.tfrac = FIXED16_FRAC(r_tstepy + r_tstepx * d_extrastep.count);
#endif
        d_extrastep.light = d_basestep.light + working_lstepx;
        d_extrastep.zi = d_basestep.zi + r_zistepx;

        D_PolysetScanLeftEdge(initialleftheight);
    }

    //
    // scan out the bottom part of the left edge, if it exists
    //
    if (pedgetable->numLeftEdges == 2) {
        plefttop = pleftbottom;
        pleftbottom = pedgetable->pLeftEdgeVert[2];

        int height = pleftbottom->y - plefttop->y;

        // TODO: make this a function; modularize this function in general

        _yStart = plefttop->y;
        d_snap.count = plefttop->x - prighttop->x;
        d_snap.ptex = (TypeLess_ptr)(
            r_affinetridesc.pskin +
            FIXED16_TO_INT(plefttop->s) +
            FIXED16_TO_INT(plefttop->t) * r_affinetridesc.skinwidth
            );
        d_snap.sfrac = 0;
        d_snap.tfrac = 0;
        d_snap.light = plefttop->light;
        d_snap.zi = plefttop->zi;

        d_snap.pdest = (TypeLess_ptr)d_viewbuffer + _yStart * screenwidth + plefttop->x;
        d_snap.pz = d_pzbuffer + _yStart * d_zwidth + plefttop->x;

        if (height == 1) {

            *d_pedgespanpackage = d_snap;
            d_pedgespanpackage++;
        }
        else {
            D_PolysetSetUpForLineScan(
                plefttop->x, plefttop->y,
                pleftbottom->x, pleftbottom->y
            );

            d_extrastep.count = d_basestep.count + 1;

            d_basestep.pdest = (TypeLess_ptr)(intptr_t)(screenwidth + d_basestep.count);
            d_extrastep.pdest = (TypeLess_ptr)(intptr_t)(screenwidth + d_extrastep.count);

#if id386
            d_basestep.pz = (int16_p)(d_zwidth + d_basestep.count) << 1;
            d_extrastep.pz = d_basestep.pz + 2;
#else
            d_basestep.pz = (int16_p)(intptr_t)(d_zwidth + d_basestep.count);
            d_extrastep.pz = (int16_p)(intptr_t)(d_zwidth + d_extrastep.count);
#endif

            int working_lstepx = (d_basestep.count < 0) ?
                (r_lstepx - 1) : r_lstepx;
            d_basestep.ptex = (TypeLess_ptr)(
                FIXED16_TO_INT(r_sstepy + r_sstepx * d_basestep.count) +
                FIXED16_TO_INT(r_tstepy + r_tstepx * d_basestep.count) * r_affinetridesc.skinwidth
                );
#if id386
            d_basestep.sfrac = (r_sstepy + r_sstepx * d_basestep.count) << 16;
            d_basestep.tfrac = (r_tstepy + r_tstepx * d_basestep.count) << 16;
#else
            d_basestep.sfrac = FIXED16_FRAC(r_sstepy + r_sstepx * d_basestep.count);
            d_basestep.tfrac = FIXED16_FRAC(r_tstepy + r_tstepx * d_basestep.count);
#endif
            d_basestep.light = r_lstepy + working_lstepx * d_basestep.count;
            d_basestep.zi = r_zistepy + r_zistepx * d_basestep.count;

            d_extrastep.ptex = (TypeLess_ptr)(
                FIXED16_TO_INT(r_sstepy + r_sstepx * d_extrastep.count) +
                FIXED16_TO_INT(r_tstepy + r_tstepx * d_extrastep.count) * r_affinetridesc.skinwidth
                );
#if id386
            d_extrastep.sfrac = (FIXED16_FRAC(r_sstepy + r_sstepx * d_extrastep.count)) << 16;
            d_extrastep.tfrac = (FIXED16_FRAC(r_tstepy + r_tstepx * d_extrastep.count)) << 16;
#else
            d_extrastep.sfrac = FIXED16_FRAC(r_sstepy + r_sstepx * d_extrastep.count);
            d_extrastep.tfrac = FIXED16_FRAC(r_tstepy + r_tstepx * d_extrastep.count);
#endif
            d_extrastep.light = d_basestep.light + working_lstepx;
            d_extrastep.zi = d_basestep.zi + r_zistepx;

            D_PolysetScanLeftEdge(height);
        }
    }

    // scan out the top (and possibly only) part of the right edge, updating the count field
    d_pedgespanpackage = a_spans;

    D_PolysetSetUpForLineScan(
        prighttop->x, prighttop->y,
        prightbottom->x, prightbottom->y
    );
    d_snap.count = 0;
    d_extrastep.count = d_basestep.count + 1;
    int originalcount = a_spans[initialrightheight].count;
    a_spans[initialrightheight].count = -999999; // mark end of the spanpackages
    D_PolysetDrawSpans8(a_spans);

    // scan out the bottom part of the right edge, if it exists
    if (pedgetable->numRightEdges == 2) {
        SpanPackage_p pstart = a_spans + initialrightheight;
        pstart->count = originalcount;

        d_snap.count = prightbottom->x - prighttop->x;

        prighttop = prightbottom;
        prightbottom = pedgetable->pRightEdgeVert[2];

        int height = prightbottom->y - prighttop->y;

        D_PolysetSetUpForLineScan(
            prighttop->x, prighttop->y,
            prightbottom->x, prightbottom->y
        );

        d_extrastep.count = d_basestep.count + 1;
        a_spans[initialrightheight + height].count = -999999;
        // mark end of the spanpackages
        D_PolysetDrawSpans8(pstart);
    }
}


#if 0

void D_PolysetRecursiveDrawLine(VertAttr_p lp1, VertAttr_p lp2) {
    {
        int d;
        if (
            (((d = (lp2->x - lp1->x)) < -1) ||
                (d > 1)) ||
            (((d = (lp2->y - lp1->y)) < -1) ||
                (d > 1))
            )
            goto split;
    }
    return; // line is completed

split:
    // split this edge
    VertAttr_t  _new = {
        .x = FIXED_MID(lp1->x, lp2->x),
        .y = FIXED_MID(lp1->y, lp2->y),
        .s = FIXED_MID(lp1->s, lp2->s),
        .t = FIXED_MID(lp1->t, lp2->t),
        .light = FIXED_MID(lp1->light, lp2->light),
        .zi = FIXED_MID(lp1->zi, lp2->zi)
    };

    // draw the point
    int ofs = d_scantable[_new.y] + _new.x;
    if (_new.zi > d_pzbuffer[ofs]) {
        d_pzbuffer[ofs] = _new.zi;
        int pix = skintable[FIXED16_TO_INT(_new.t)][FIXED16_TO_INT(_new.s)];
        //  pix = ((uint8_t *)acolormap)[pix + (_new.light & 0xFF00)];
        d_viewbuffer[ofs] = pix;
    }

    // recursively continue
    D_PolysetRecursiveDrawLine(lp1, &_new);
    D_PolysetRecursiveDrawLine(&_new, lp2);
}

void D_PolysetRecursiveTriangle2(VertAttr_p lp1, VertAttr_p lp2, VertAttr_p lp3) {
    {
        int d;
        if (
            (((d = (lp2->x - lp1->x)) < -1) ||
                (d > 1)) ||
            (((d = (lp2->y - lp1->y)) < -1) ||
                (d > 1))
            )
            goto split;
    }
    return;

split:
    // split this edge
    VertAttr_t  _new = {
        .x = FIXED_MID(lp1->x, lp2->x),
        .y = FIXED_MID(lp1->y, lp2->y),
        .s = FIXED_MID(lp1->s, lp2->s),
        .t = FIXED_MID(lp1->t, lp2->t),
        .light = FIXED_MID(lp1->light, lp2->light),
        .zi = FIXED_MID(lp1->zi, lp2->zi)
    };

    D_PolysetRecursiveDrawLine(&_new, lp3);

    // recursively continue
    D_PolysetRecursiveTriangle(lp1, &_new, lp3);
    D_PolysetRecursiveTriangle(&_new, lp2, lp3);
}

#endif

