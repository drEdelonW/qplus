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
// d_scan.c
//
// Portable C scan-level rasterization code, all pixel depths.

#include "r_local.h"
#include "d_local.h"
#include "screen.h"

// Projective 1/z, float domain -- carried alongside sz/tz until the perspective
// divide; on its own (without sz/tz) it means nothing.
typedef float InvZf;

// 1/z prescaled into the z-buffer's own fixed radix (see D_DrawZSpans: scale is
// 0x8000 * FIXED16_ONE). A different fixed-point domain from fixed16_t s/t and
// from InvZf -- do not mix with either.
typedef fixed16_t InvZq;

// Homogeneous perspective-divide carrier (s/z, t/z, 1/z), affine across the
// screen (u,v), not across texture space. sz/tz are meaningless without zi --
// keep them together, never split zi off into its own variable.
typedef struct {
    float sz;
    float tz;
    InvZf zi;
} DividedST_t;

// Texel-space s/t, fixed16_t Q16.16, valid only after the perspective divide.
// Affine within one span; re-anchored by division at each span boundary.
typedef struct {
    fixed16_t s;
    fixed16_t t;
} STq16_t;

static uint8_p r_turb_pbase;
static uint8_p r_turb_pdest;
STq16_t r_turb_st;
STq16_t r_turb_ststep;
int* r_turb_turb;
int r_turb_spancount;

void D_DrawTurbulent8Span();


/*
=============
D_WarpScreen

// this performs a slight compression of the screen at the same time as
// the sine warp, to keep the edges from wrapping
=============
*/
void D_WarpScreen() {   // Under water warp
    int* turb;
    int* col;
    uint8_p rowptr[MAXHEIGHT + (AMP2 * 2)];
    int     column[MAXWIDTH + (AMP2 * 2)];

    int w = r_refdef.vrect.width;
    int h = r_refdef.vrect.height;

    float wratio = w / (float)scr.vrect.width;
    float hratio = h / (float)scr.vrect.height;

    for (int v = 0; v < scr.vrect.height + AMP2 * 2; v++) {
        rowptr[v] = d_viewbuffer + (r_refdef.vrect.y * screenwidth) +
            (screenwidth * (int)((float)v * hratio * h / (h + AMP2 * 2)));
    }

    for (int u = 0; u < scr.vrect.width + AMP2 * 2; u++) {
        column[u] = r_refdef.vrect.x +
            (int)((float)u * wratio * w / (w + AMP2 * 2));
    }

    turb = intsintable + ((int)(GetClSimTime() * SPEED) & (CYCLE - 1));
    uint8_p dest = vid.scr.pBuff + scr.vrect.y * vid.rowbytes + scr.vrect.x;

    for (int v = 0; v < scr.vrect.height; v++, dest += vid.rowbytes) {
        col = &column[turb[v]];
        uint8_ar row = &rowptr[v];

        for (int u = 0; u < scr.vrect.width; u += 4) {
            dest[u + 0] = row[turb[u + 0]][col[u + 0]];
            dest[u + 1] = row[turb[u + 1]][col[u + 1]];
            dest[u + 2] = row[turb[u + 2]][col[u + 2]];
            dest[u + 3] = row[turb[u + 3]][col[u + 3]];
        }
    }
}


#if    !id386

/*
=============
D_DrawTurbulent8Span
=============
*/
void D_DrawTurbulent8Span() {
    do {
        fixed16_t sturb = FIXED16_TO_INT(r_turb_st.s + r_turb_turb[FIXED16_TO_INT(r_turb_st.t) & (CYCLE - 1)]) & 0x3F;
        fixed16_t tturb = FIXED16_TO_INT(r_turb_st.t + r_turb_turb[FIXED16_TO_INT(r_turb_st.s) & (CYCLE - 1)]) & 0x3F;
        *r_turb_pdest++ = *(r_turb_pbase + MUL64(tturb) + sturb);
        r_turb_st.s += r_turb_ststep.s;
        r_turb_st.t += r_turb_ststep.t;
    } while (--r_turb_spancount > 0);
}

#endif    // !id386


/*
=============
Turbulent8
=============
*/
void Turbulent8(eSpan_p pspan) {
    r_turb_turb = sintable + ((int)(GetClSimTime() * SPEED) & (CYCLE - 1));

    r_turb_ststep.s = 0;    // keep compiler happy
    r_turb_ststep.t = 0;    // ditto

    r_turb_pbase = (uint8_p)cacheblock;

    DividedST_t dz16step = {
        .sz = d_sdivzstepu * 16.f,
        .tz = d_tdivzstepu * 16.f,
        .zi = d_zistepu * 16.f,
    };

    do {
        r_turb_pdest = (uint8_p)(
            (uint8_p)d_viewbuffer +
            (screenwidth * pspan->v) + pspan->u);

        int count = pspan->count;

        // calculate the initial s/z, t/z, 1/z, s, and t and clamp
        float du = (float)pspan->u;
        float dv = (float)pspan->v;

        DividedST_t dz = {
            .sz = d_sdivzorigin + dv * d_sdivzstepv + du * d_sdivzstepu,
            .tz = d_tdivzorigin + dv * d_tdivzstepv + du * d_tdivzstepu,
            .zi = d_ziorigin + dv * d_zistepv + du * d_zistepu,
        };
        InvZf z = (InvZf)FIXED16_ONE / dz.zi;    // prescale to 16.16 fixed-point

        r_turb_st.s = (int)(dz.sz * z) + sadjust;
        /**/ if (r_turb_st.s > bbextents)      r_turb_st.s = bbextents;
        else if (r_turb_st.s < 0)              r_turb_st.s = 0;

        r_turb_st.t = (int)(dz.tz * z) + tadjust;
        /**/ if (r_turb_st.t > bbextentt)      r_turb_st.t = bbextentt;
        else if (r_turb_st.t < 0)              r_turb_st.t = 0;

        do {
            // calculate s and t at the far end of the span
            r_turb_spancount = (count >= 16) ? 16 : count;

            count -= r_turb_spancount;
            STq16_t st_next;
            if (count) {
                // calculate s/z, t/z, zi->fixed s and t at far end of span,
                // calculate s and t steps across span by shifting
                dz.sz += dz16step.sz;
                dz.tz += dz16step.tz;
                dz.zi += dz16step.zi;
                InvZf z = (InvZf)FIXED16_ONE / dz.zi;    // prescale to 16.16 fixed-point

                st_next.s = (int)(dz.sz * z) + sadjust;
                /**/ if (st_next.s > bbextents)     st_next.s = bbextents;
                else if (st_next.s < 16)            st_next.s = 16;    // prevent round-off error on <0 steps from
                //  from causing overstepping & running off the edge of the texture

                st_next.t = (int)(dz.tz * z) + tadjust;
                /**/ if (st_next.t > bbextentt)     st_next.t = bbextentt;
                else if (st_next.t < 16)            st_next.t = 16;    // guard against round-off error on <0 steps

                r_turb_ststep.s = FIXED4_TO_INT(st_next.s - r_turb_st.s);
                r_turb_ststep.t = FIXED4_TO_INT(st_next.t - r_turb_st.t);
            }
            else {
                // calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
                // can't step off polygon), clamp, calculate s and t steps across
                // span by division, biasing steps low so we don't run off the
                // texture
                float spancountminus1 = (float)(r_turb_spancount - 1);
                dz.sz += d_sdivzstepu * spancountminus1;
                dz.tz += d_tdivzstepu * spancountminus1;
                dz.zi += d_zistepu * spancountminus1;
                InvZf z = (InvZf)FIXED16_ONE / dz.zi;    // prescale to 16.16 fixed-point
                st_next.s = (int)(dz.sz * z) + sadjust;
                /**/ if (st_next.s > bbextents)     st_next.s = bbextents;
                else if (st_next.s < 16)            st_next.s = 16;    // prevent round-off error on <0 steps from
                //  from causing overstepping & running off the edge of the texture

                st_next.t = (int)(dz.tz * z) + tadjust;
                /**/ if (st_next.t > bbextentt)     st_next.t = bbextentt;
                else if (st_next.t < 16)            st_next.t = 16;    // guard against round-off error on <0 steps

                if (r_turb_spancount > 1) {
                    r_turb_ststep.s = (st_next.s - r_turb_st.s) / (r_turb_spancount - 1);
                    r_turb_ststep.t = (st_next.t - r_turb_st.t) / (r_turb_spancount - 1);
                }
            }

            r_turb_st.s = r_turb_st.s & (INT_TO_FIXED16(CYCLE) - 1);
            r_turb_st.t = r_turb_st.t & (INT_TO_FIXED16(CYCLE) - 1);

            D_DrawTurbulent8Span();

            r_turb_st = st_next;

        } while (count > 0);

    } while ((pspan = pspan->pnext) != NULL);
}


#if    !id386

/*
=============
D_DrawSpans8
=============
*/
void D_DrawSpans8(eSpan_p pspan) {
    uint8_p pbase = (uint8_p)cacheblock;

    float sdivz8stepu = d_sdivzstepu * 8.0f;
    float tdivz8stepu = d_tdivzstepu * 8.0f;
    float zi8stepu = d_zistepu * 8.0f;

    do {
        uint8_p pdest = (uint8_p)(
            (uint8_p)d_viewbuffer +
            (screenwidth * pspan->v) + pspan->u);

        int count = pspan->count;

        // calculate the initial s/z, t/z, 1/z, s, and t and clamp
        float du = (float)pspan->u;
        float dv = (float)pspan->v;

        DividedST_t dz = {
            .sz = d_sdivzorigin + dv * d_sdivzstepv + du * d_sdivzstepu,
            .tz = d_tdivzorigin + dv * d_tdivzstepv + du * d_tdivzstepu,
            .zi = d_ziorigin + dv * d_zistepv + du * d_zistepu,
        };
        InvZf z = (InvZf)FIXED16_ONE / dz.zi;    // prescale to 16.16 fixed-point

        STq16_t st;
        st.s = (int)(dz.sz * z) + sadjust;
        /**/ if (st.s > bbextents)     st.s = bbextents;
        else if (st.s < 0)             st.s = 0;

        st.t = (int)(dz.tz * z) + tadjust;
        /**/ if (st.t > bbextentt)     st.t = bbextentt;
        else if (st.t < 0)             st.t = 0;

        do {
            // calculate s and t at the far end of the span
            int spancount = (count >= 8) ?
                8 : count;

            count -= spancount;
            STq16_t ststep = { .s = 0, .t = 0 };    // keep compiler happy
            STq16_t st_next;
            if (count) {
                // calculate s/z, t/z, zi->fixed s and t at far end of span,
                // calculate s and t steps across span by shifting
                dz.sz += sdivz8stepu;
                dz.tz += tdivz8stepu;
                dz.zi += zi8stepu;
                InvZf z = (InvZf)FIXED16_ONE / dz.zi;    // prescale to 16.16 fixed-point

                st_next.s = (int)(dz.sz * z) + sadjust;
                /**/ if (st_next.s > bbextents)         st_next.s = bbextents;
                else if (st_next.s < 8)                 st_next.s = 8;    // prevent round-off error on <0 steps from
                //  from causing overstepping & running off the  edge of the texture

                st_next.t = (int)(dz.tz * z) + tadjust;
                /**/ if (st_next.t > bbextentt)         st_next.t = bbextentt;
                else if (st_next.t < 8)                 st_next.t = 8;    // guard against round-off error on <0 steps

                ststep.s = EIGHTH(st_next.s - st.s);
                ststep.t = EIGHTH(st_next.t - st.t);
            }
            else {
                // calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
                // can't step off polygon), clamp, calculate s and t steps across
                // span by division, biasing steps low so we don't run off the
                // texture
                float spancountminus1 = (float)(spancount - 1);
                dz.sz += d_sdivzstepu * spancountminus1;
                dz.tz += d_tdivzstepu * spancountminus1;
                dz.zi += d_zistepu * spancountminus1;
                InvZf z = (InvZf)FIXED16_ONE / dz.zi;    // prescale to 16.16 fixed-point
                st_next.s = (int)(dz.sz * z) + sadjust;
                /**/ if (st_next.s > bbextents)     st_next.s = bbextents;
                else if (st_next.s < 8)             st_next.s = 8;    // prevent round-off error on <0 steps from
                //  from causing overstepping & running off the
                //  edge of the texture

                st_next.t = (int)(dz.tz * z) + tadjust;
                /**/ if (st_next.t > bbextentt)     st_next.t = bbextentt;
                else if (st_next.t < 8)             st_next.t = 8;    // guard against round-off error on <0 steps

                if (spancount > 1) {
                    ststep.s = (st_next.s - st.s) / (spancount - 1);
                    ststep.t = (st_next.t - st.t) / (spancount - 1);
                }
            }

            do {
                *pdest++ = *(pbase + FIXED16_TO_INT(st.s) + FIXED16_TO_INT(st.t) * cachewidth);
                st.s += ststep.s;
                st.t += ststep.t;
            } while (--spancount > 0);

            st = st_next;

        } while (count > 0);

    } while ((pspan = pspan->pnext) != NULL);
}

#endif


#if    !id386

/*
=============
D_DrawZSpans
=============
*/
void D_DrawZSpans(eSpan_p pspan) {
    int doublecount;

    // FIXME: check for clamping/range problems
    // we count on FP exceptions being turned off to avoid range problems
    // izi/izistep are InvZq: 1/z prescaled by 0x8000*FIXED16_ONE, a different
    // fixed radix from the texture-domain fixed16_t s/t and from InvZf.
    InvZq izistep = (InvZq)(d_zistepu * 0x8000 * FIXED16_ONE);

    do {
        int16_p pdest = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

        int count = pspan->count;

        // calculate the initial 1/z
        float du = (float)pspan->u;
        float dv = (float)pspan->v;

        double zi = d_ziorigin + dv * d_zistepv + du * d_zistepu;
        // we count on FP exceptions being turned off to avoid range problems
        InvZq izi = (InvZq)(zi * 0x8000 * FIXED16_ONE);

        if (((uintptr_t)pdest) & 0x02u) {
            *pdest++ = (int16_t)(FIXED16_TO_INT(izi));
            izi += izistep;
            count--;
        }

        if ((doublecount = HALF(count)) > 0) {
            do {
                fixed16_t ltemp = FIXED16_TO_INT(izi);
                izi += izistep;
                ltemp |= izi & 0xFFFF0000;
                izi += izistep;
                *(int*)pdest = ltemp;
                pdest += 2;
            } while (--doublecount > 0);
        }

        if (count & 1)
            *pdest = (int16_t)(FIXED16_TO_INT(izi));

    } while ((pspan = pspan->pnext) != NULL);
}

#endif

