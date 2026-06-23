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
    TypeLess_ptr    pdest;
    int16_p         pz;
    int             count;
    uint8_p         ptex;
    fixed16_t       sfrac;
    fixed16_t       tfrac;
    fixed16_t       light;
    fixed16_t       zi;
} SpanPackage_t;
typedef SpanPackage_t* SpanPackage_p;

typedef struct {
    int  isflattop;     // TODO: not used
    int  numLeftEdges;
    VertAttr_p pLeftEdgeVert[3];
    int  numRightEdges;
    VertAttr_p pRightEdgeVert[3];
} EdgeTable_t;
typedef EdgeTable_t* EdgeTable_p;

VertAttr_t r_p0, r_p1, r_p2;

uint8_p d_pcolormap;

int   d_aflatcolor;
int   d_xdenom;

EdgeTable_p pedgetable;

EdgeTable_t edgetables[12] = {
    {0, 1, {&r_p0, &r_p2, NULL},  2, {&r_p0, &r_p1, &r_p2}},
    {0, 2, {&r_p1, &r_p0, &r_p2}, 1, {&r_p1, &r_p2, NULL }},
    {1, 1, {&r_p0, &r_p2, NULL},  1, {&r_p1, &r_p2, NULL }},
    {0, 1, {&r_p1, &r_p0, NULL},  2, {&r_p1, &r_p2, &r_p0}},
    {0, 2, {&r_p0, &r_p2, &r_p1}, 1, {&r_p0, &r_p1, NULL }},
    {0, 1, {&r_p2, &r_p1, NULL},  1, {&r_p2, &r_p0, NULL }},
    {0, 1, {&r_p2, &r_p1, NULL},  2, {&r_p2, &r_p0, &r_p1}},
    {0, 2, {&r_p2, &r_p1, &r_p0}, 1, {&r_p2, &r_p0, NULL }},
    {0, 1, {&r_p1, &r_p0, NULL},  1, {&r_p1, &r_p2, NULL }},
    {1, 1, {&r_p2, &r_p1, NULL},  1, {&r_p0, &r_p1, NULL }},
    {1, 1, {&r_p1, &r_p0, NULL},  1, {&r_p2, &r_p0, NULL }},
    {0, 1, {&r_p0, &r_p2, NULL},  1, {&r_p0, &r_p1, NULL }},
};

// FIXME: some of these can become statics
int a_sstepxfrac, a_tstepxfrac;
int r_sstepx, r_tstepx, r_lstepx, r_zistepx;    // s t l zi
int r_sstepy, r_tstepy, r_lstepy, r_zistepy;    // s t l zi

int a_ststepxwhole;

SpanPackage_p a_spans;
SpanPackage_p d_pedgespanpackage;
static int    _yStart;

#if 0
uint8_p d_pdest;
int16_p d_pz;
int d_aspancount;
uint8_p d_ptex;
fixed16_t d_sfrac;
fixed16_t d_tfrac;
fixed16_t d_light;
fixed16_t d_zi;    // s t l zi
#else
SpanPackage_t d_snap;
#endif

int d_pdestbasestep;//, d_pdestextrastep;
int d_pzbasestep;//, d_pzextrastep;
int ubasestep;//, d_countextrastep;
int d_ptexbasestep;//, d_ptexextrastep;
fixed16_t d_sfracbasestep;//, d_sfracextrastep;  // s
fixed16_t d_tfracbasestep;//, d_tfracextrastep;  // t
fixed16_t d_lightbasestep;//, d_lightextrastep;  // l
fixed16_t d_zibasestep;//, d_ziextrastep;        //zi

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
void D_PolysetSetEdgeTable();
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
        // valid triangle coordinates for filling can include the bottom and
        // right clip edges, due to the fill rule; these shouldn't be drawn
        if ((fv->vAttr.x < r_refdef.vrectright) &&
            (fv->vAttr.y < r_refdef.vrectbottom)
            ) {
            int z = FIXED16_TO_INT(fv->vAttr.zi);
            int16_p zbuf = zspantable[fv->vAttr.y] + fv->vAttr.x;
            if (z >= *zbuf) {
                int  pix;

                *zbuf = z;
                pix = skintable[FIXED16_TO_INT(fv->vAttr.t)][FIXED16_TO_INT(fv->vAttr.s)];
                pix = ((uint8_p)acolormap)[pix + (fv->vAttr.light & 0xFF00)];
                d_viewbuffer[d_scantable[fv->vAttr.y] + fv->vAttr.x] = pix;
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

        d_pcolormap = &((uint8_p)acolormap)[index0->vAttr.light & 0xFF00];

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

        d_xdenom = (index0->vAttr.y - index1->vAttr.y) *
            (index0->vAttr.x - index2->vAttr.x) -
            (index0->vAttr.x - index1->vAttr.x) * (index0->vAttr.y - index2->vAttr.y);

        if (d_xdenom >= 0) { continue; }

        r_p0 = index0->vAttr;
        r_p1 = index1->vAttr;
        r_p2 = index2->vAttr;

        if (!ptri->facesfront) {
            if (index0->flags & ALIAS_ONSEAM)   r_p0.s += r_affinetridesc.seamfixupX16;
            if (index1->flags & ALIAS_ONSEAM)   r_p1.s += r_affinetridesc.seamfixupX16;
            if (index2->flags & ALIAS_ONSEAM)   r_p2.s += r_affinetridesc.seamfixupX16;
        }

        D_PolysetSetEdgeTable();
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
        .x = (lp1->x + lp2->x) >> 1,
        .y = (lp1->y + lp2->y) >> 1,
        .s = (lp1->s + lp2->s) >> 1,
        .t = (lp1->t + lp2->t) >> 1,
        /* .light — skipped */
        .zi = (lp1->zi + lp2->zi) >> 1,
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
            int pix;

            *zbuf = z;
            pix = d_pcolormap[skintable[FIXED16_TO_INT(_new.t)][FIXED16_TO_INT(_new.s)]];
            d_viewbuffer[d_scantable[_new.y] + _new.x] = pix;
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
#if 0
        * d_pedgespanpackage = (SpanPackage_t){
            .pdest = d_pdest,
            .pz = d_pz,
            .count = d_aspancount,
            .ptex = d_ptex,
            .sfrac = d_sfrac,
            .tfrac = d_tfrac,
            // FIXME: need to clamp l, s, t, at both ends?
            .light = d_light,
            .zi = d_zi
        };
#else
        * d_pedgespanpackage = d_snap;
#endif

        d_pedgespanpackage++;
        errorterm += erroradjustup;
        if (errorterm >= 0) {
            d_snap.pdest += (int)d_extrastep.pdest; // TODO: can be address issue. take care!
            d_snap.pz += (int)d_extrastep.pz;
            d_snap.count += d_extrastep.count;
            d_snap.ptex += (int)d_extrastep.ptex;
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
            d_snap.pdest += d_pdestbasestep;
            d_snap.pz += d_pzbasestep;
            d_snap.count += ubasestep;
            d_snap.ptex += d_ptexbasestep;
            d_snap.sfrac += d_sfracbasestep;
            d_snap.ptex += FIXED16_TO_INT(d_snap.sfrac);
            d_snap.sfrac &= 0xFFFF;
            d_snap.tfrac += d_tfracbasestep;
            if (d_snap.tfrac & 0x10000) {
                d_snap.ptex += r_affinetridesc.skinwidth;
                d_snap.tfrac &= 0xFFFF;
            }
            d_snap.light += d_lightbasestep;
            d_snap.zi += d_zibasestep;
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
        ((tn <= 16) && (tn >= -15))) {
        adivtab_p ptemp = &_aDivTab[((tm + 15) << 5) + (tn + 15)];
        ubasestep = ptemp->quotient;
        erroradjustup = ptemp->remainder;
        erroradjustdown = tn;
    }
    else {
        double dm = (double)tm;
        double dn = (double)tn;

        FloorDivMod(dm, dn, &ubasestep, &erroradjustup);

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
    float p00_minus_p20 = r_p0.x - r_p2.x;
    float p01_minus_p21 = r_p0.y - r_p2.y;
    float p10_minus_p20 = r_p1.x - r_p2.x;
    float p11_minus_p21 = r_p1.y - r_p2.y;

    float xstepdenominv = 1.0 / (float)d_xdenom;

    float ystepdenominv = -xstepdenominv;

    // ceil() for light so positive steps are exaggerated, negative steps
    // diminished,  pushing us away from underflow toward overflow. Underflow is
    // very visible, overflow is very unlikely, because of ambient lighting
    float t0 = r_p0.light - r_p2.light;
    float t1 = r_p1.light - r_p2.light;
    r_lstepx = (int)ceil((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
    r_lstepy = (int)ceil((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

    t0 = r_p0.s - r_p2.s;
    t1 = r_p1.s - r_p2.s;
    r_sstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
    r_sstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

    t0 = r_p0.t - r_p2.t;
    t1 = r_p1.t - r_p2.t;
    r_tstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
    r_tstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

    t0 = r_p0.zi - r_p2.zi;
    t1 = r_p1.zi - r_p2.zi;
    r_zistepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
    r_zistepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

#if id386
    a_sstepxfrac = r_sstepx << 16;
    a_tstepxfrac = r_tstepx << 16;
#else
    a_sstepxfrac = r_sstepx & 0xFFFF;
    a_tstepxfrac = r_tstepx & 0xFFFF;
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
        else d_snap.count += ubasestep;


        if (lcount) {
            uint8_p lpdest = pspanpackage->pdest;
            uint8_p lptex = pspanpackage->ptex;
            int16_p lpz = pspanpackage->pz;
            fixed16_t lsfrac = pspanpackage->sfrac;
            int ltfrac = pspanpackage->tfrac;
            int llight = pspanpackage->light;
            fixed16_t lzi = pspanpackage->zi;

            do {
                if (FIXED16_TO_INT(lzi) >= *lpz) {
                    *lpdest = ((uint8_p)acolormap)[*lptex + (llight & 0xFF00)];
                    // gel mapping     *lpdest = gelmap[*lpdest];
                    *lpz = FIXED16_TO_INT(lzi);
                }
                lpdest++;
                lzi += r_zistepx;
                lpz++;
                llight += r_lstepx;
                lptex += a_ststepxwhole;
                lsfrac += a_sstepxfrac;
                lptex += FIXED16_TO_INT(lsfrac);
                lsfrac &= 0xFFFF;
                ltfrac += a_tstepxfrac;
                if (ltfrac & 0x10000) {
                    lptex += r_affinetridesc.skinwidth;
                    ltfrac &= 0xFFFF;
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

    //
    // set the s, t, and light gradients, which are consistent across the triangle
    // because being a triangle, things are affine
    //
    D_PolysetCalcGradients(r_affinetridesc.skinwidth);

    //
    // rasterize the polygon
    //

    //
    // scan out the top (and possibly only) part of the left edge
    //
    d_pedgespanpackage = a_spans;

    _yStart = plefttop->y;
    d_snap.count = plefttop->x - prighttop->x;

    d_snap.ptex = (uint8_p)r_affinetridesc.pskin +
        FIXED16_TO_INT(plefttop->s) +
        FIXED16_TO_INT(plefttop->t) * r_affinetridesc.skinwidth;
#if id386
    d_snap.sfrac = (plefttop.s & 0xFFFF) << 16;
    d_snap.tfrac = (plefttop.t & 0xFFFF) << 16;
#else
    d_snap.sfrac = plefttop->s & 0xFFFF;
    d_snap.tfrac = plefttop->t & 0xFFFF;
#endif
    d_snap.light = plefttop->light;
    d_snap.zi = plefttop->zi;

    d_snap.pdest = (uint8_p)d_viewbuffer + plefttop->x + (_yStart * screenwidth);
    d_snap.pz = d_pzbuffer + _yStart * d_zwidth + plefttop->x;

    if (initialleftheight == 1) {

#if 0
        * d_pedgespanpackage = (SpanPackage_t){
            .pdest = d_pdest,
            .pz = d_pz,
            .count = d_aspancount,
            .ptex = d_ptex,
            .sfrac = d_sfrac,
            .tfrac = d_tfrac,
            // FIXME: need to clamp l, s, t, at both ends?
            .light = d_light,
            .zi = d_zi
        };
#else
        * d_pedgespanpackage = d_snap;
#endif
        d_pedgespanpackage++;
    }
    else {
        D_PolysetSetUpForLineScan(
            plefttop->x, plefttop->y,
            pleftbottom->x, pleftbottom->y
        );

#if id386
        d_pzbasestep = (d_zwidth + ubasestep) << 1;
        d_pzextrastep = d_pzbasestep + 2;
#else
        d_pzbasestep = d_zwidth + ubasestep;
        d_extrastep.pz = (int16_p)d_pzbasestep + 1;
#endif

        d_pdestbasestep = screenwidth + ubasestep;
        d_extrastep.pdest = (TypeLess_ptr)d_pdestbasestep + 1;

        // TODO: can reuse partial expressions here

        // for negative steps in x along left edge, bias toward overflow rather than
        // underflow (sort of turning the floor() we did in the gradient calcs into
        // ceil(), but plus a little bit)
        int working_lstepx = (ubasestep < 0) ?
            (r_lstepx - 1) : r_lstepx;

        d_extrastep.count = ubasestep + 1;
        d_ptexbasestep =
            ((r_sstepy + r_sstepx * ubasestep) >> 16) +
            ((r_tstepy + r_tstepx * ubasestep) >> 16) * r_affinetridesc.skinwidth;
#if id386
        d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
        d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
#else
        d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
        d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
#endif
        d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
        d_zibasestep = r_zistepy + r_zistepx * ubasestep;

        d_extrastep.ptex = (uint8_p)(
            ((r_sstepy + r_sstepx * d_extrastep.count) >> 16) +
            ((r_tstepy + r_tstepx * d_extrastep.count) >> 16) * r_affinetridesc.skinwidth
            );
#if id386
        d_extrastep.sfrac = (r_sstepy + r_sstepx * d_countextrastep) << 16;
        d_extrastep.tfrac = (r_tstepy + r_tstepx * d_countextrastep) << 16;
#else
        d_extrastep.sfrac = (r_sstepy + r_sstepx * d_extrastep.count) & 0xFFFF;
        d_extrastep.tfrac = (r_tstepy + r_tstepx * d_extrastep.count) & 0xFFFF;
#endif
        d_extrastep.light = d_lightbasestep + working_lstepx;
        d_extrastep.zi = d_zibasestep + r_zistepx;

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
        d_snap.ptex = (uint8_p)r_affinetridesc.pskin +
            FIXED16_TO_INT(plefttop->s) +
            FIXED16_TO_INT(plefttop->t) * r_affinetridesc.skinwidth;
        d_snap.sfrac = 0;
        d_snap.tfrac = 0;
        d_snap.light = plefttop->light;
        d_snap.zi = plefttop->zi;

        d_snap.pdest = (uint8_p)d_viewbuffer + _yStart * screenwidth + plefttop->x;
        d_snap.pz = d_pzbuffer + _yStart * d_zwidth + plefttop->x;

        if (height == 1) {
#if 0
            * d_pedgespanpackage = (SpanPackage_t){
                .pdest = d_pdest,
                .pz = d_pz,
                .count = d_aspancount,
                .ptex = d_ptex,
                .sfrac = d_sfrac,
                .tfrac = d_tfrac,
                // FIXME: need to clamp l, s, t, at both ends?
                .light = d_light,
                .zi = d_zi
        };
#else
            * d_pedgespanpackage = d_snap;
#endif
            d_pedgespanpackage++;
    }
        else {
            D_PolysetSetUpForLineScan(
                plefttop->x, plefttop->y,
                pleftbottom->x, pleftbottom->y
            );

            d_pdestbasestep = screenwidth + ubasestep;
            d_extrastep.pdest = (uint8_p)d_pdestbasestep + 1;

#if id386
            d_pzbasestep = (d_zwidth + ubasestep) << 1;
            d_extrastep.pz = (int16_p)d_pzbasestep + 2;
#else
            d_pzbasestep = d_zwidth + ubasestep;
            d_extrastep.pz = (int16_p)d_pzbasestep + 1;
#endif

            int working_lstepx = (ubasestep < 0) ?
                (r_lstepx - 1) : r_lstepx;

            d_extrastep.count = ubasestep + 1;
            d_ptexbasestep =
                ((r_sstepy + r_sstepx * ubasestep) >> 16) +
                ((r_tstepy + r_tstepx * ubasestep) >> 16) * r_affinetridesc.skinwidth;
#if id386
            d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
            d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
#else
            d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
            d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
#endif
            d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
            d_zibasestep = r_zistepy + r_zistepx * ubasestep;

            d_extrastep.ptex = (uint8_p)(
                ((r_sstepy + r_sstepx * d_extrastep.count) >> 16) +
                ((r_tstepy + r_tstepx * d_extrastep.count) >> 16) * r_affinetridesc.skinwidth);
#if id386
            d_sfracextrastep = ((r_sstepy + r_sstepx * d_countextrastep) & 0xFFFF) << 16;
            d_tfracextrastep = ((r_tstepy + r_tstepx * d_countextrastep) & 0xFFFF) << 16;
#else
            d_extrastep.sfrac = (r_sstepy + r_sstepx * d_extrastep.count) & 0xFFFF;
            d_extrastep.tfrac = (r_tstepy + r_tstepx * d_extrastep.count) & 0xFFFF;
#endif
            d_extrastep.light = d_lightbasestep + working_lstepx;
            d_extrastep.zi = d_zibasestep + r_zistepx;

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
    d_extrastep.count = ubasestep + 1;
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

        d_extrastep.count = ubasestep + 1;
        a_spans[initialrightheight + height].count = -999999;
        // mark end of the spanpackages
        D_PolysetDrawSpans8(pstart);
    }
}


/*
================
D_PolysetSetEdgeTable
================
*/
void D_PolysetSetEdgeTable() {
    int edgetableindex = 0; // assume the vertices are already in
    //  top to bottom order

//
// determine which edges are right & left, and the order in which
// to rasterize them
//
    if (r_p0.y >= r_p1.y) {
        if (r_p0.y == r_p1.y) {
            if (r_p0.y < r_p2.y)  pedgetable = &edgetables[2];
            else                    pedgetable = &edgetables[5];

            return;
        }
        else    edgetableindex = 1;

    }

    if (r_p0.y == r_p2.y) {
        if (edgetableindex)     pedgetable = &edgetables[8];
        else                    pedgetable = &edgetables[9];

        return;
    }
    else if (r_p1.y == r_p2.y) {
        if (edgetableindex)     pedgetable = &edgetables[10];
        else                    pedgetable = &edgetables[11];

        return;
    }

    if (r_p0.y > r_p2.y)      edgetableindex += 2;
    if (r_p1.y > r_p2.y)      edgetableindex += 4;

    pedgetable = &edgetables[edgetableindex];
}


#if 0

void D_PolysetRecursiveDrawLine(VertAttr_p lp1, VertAttr_p lp2) {
    int d = lp2->x - lp1->x;
    if ((d < -1) || (d > 1))    goto split;
    int d = lp2->.y - lp1->.y;
    if ((d < -1) || (d > 1))    goto split;

    return; // line is completed

split:
    // split this edge
    int  _new[FV_COUNT];
    _new[0] = (lp1->x + lp2->x) >> 1;
    _new[1] = (lp1->.y + lp2->.y) >> 1;
    _new[5] = (lp1[5] + lp2[5]) >> 1;
    _new[2] = (lp1[2] + lp2[2]) >> 1;
    _new[3] = (lp1[3] + lp2[3]) >> 1;
    _new[4] = (lp1[4] + lp2[4]) >> 1;

    // draw the point
    int ofs = d_scantable[_new[1]] + _new[0];
    if (_new[5] > d_pzbuffer[ofs]) {
        d_pzbuffer[ofs] = _new[5];
        int pix = skintable[FIXED16_TO_INT(_new.t)][FIXED16_TO_INT(_new.s)];
        //  pix = ((uint8_t *)acolormap)[pix + (_new[4] & 0xFF00)];
        d_viewbuffer[ofs] = pix;
    }

    // recursively continue
    D_PolysetRecursiveDrawLine(lp1, _new);
    D_PolysetRecursiveDrawLine(_new, lp2);
}

void D_PolysetRecursiveTriangle2(VertAttr_p lp1, VertAttr_p lp2, VertAttr_p lp3) {
    int d = lp2->x - lp1->x;
    if ((d < -1) || (d > 1))    goto split;
    d = lp2->.y - lp1->.y;
    if ((d < -1) || (d > 1))    goto split;
    return;

split:
    // split this edge
    int _new[FV_COUNT];
    _new[0] = (lp1->x + lp2->x) >> 1;
    _new[1] = (lp1->.y + lp2->.y) >> 1;
    _new[5] = (lp1[5] + lp2[5]) >> 1;
    _new[2] = (lp1[2] + lp2[2]) >> 1;
    _new[3] = (lp1[3] + lp2[3]) >> 1;
    _new[4] = (lp1[4] + lp2[4]) >> 1;

    D_PolysetRecursiveDrawLine(_new, lp3);

    // recursively continue
    D_PolysetRecursiveTriangle(lp1, _new, lp3);
    D_PolysetRecursiveTriangle(_new, lp2, lp3);
}

#endif

