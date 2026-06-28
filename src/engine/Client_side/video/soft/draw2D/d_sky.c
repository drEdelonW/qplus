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
// d_sky.c

#include "r_local.h"
#include "d_local.h"

#define R_SKY_SMASK     (0x007F0000)
#define R_SKY_TMASK     (0x007F0000)

#define SKY_SPAN_SHIFT 5
#define SKY_SPAN_MAX (1 << SKY_SPAN_SHIFT)

/*
=================
D_Sky_uv_To_st
=================
*/
void D_Sky_uv_To_st(int u, int v, fixed16_p s, fixed16_p t) {
    float temp = (float)(
        (r_refdef.vrect.width >= r_refdef.vrect.height) ?
        r_refdef.vrect.width : r_refdef.vrect.height
        );

    float wu = 8192.0f * (float)(u - HALF(vid.scr.width)) / temp;
    float wv = 8192.0f * (float)(HALF(vid.scr.height) - v) / temp;


    vec3_t end = VectorMA(VectorMA(
        VectorScale(BS.forward, 4096.0f),
        wu, BS.right),
        wv, BS.up
    );
    end.z *= 3.0f;
    VectorNormalize(&end);

    {
        float temp = skytime * skyspeed; // TODO: add D_SetupFrame & set this there
        *s = (int)((temp + 6 * (HALF(SKYSIZE) - 1) * end.x) * 0x10000);
        *t = (int)((temp + 6 * (HALF(SKYSIZE) - 1) * end.y) * 0x10000);
    }
}


/*
=================
D_DrawSkyScans8
=================
*/
void D_DrawSkyScans8(eSpan_p pspan) {
    do {
        uint8_p pdest = (uint8_p)((uint8_p)d_viewbuffer +
            (screenwidth * pspan->v) + pspan->u);

        int count = pspan->count;

        fixed16_t  s, t;

        // calculate the initial s & t
        int u = pspan->u;
        int v = pspan->v;
        D_Sky_uv_To_st(u, v, &s, &t);

        do {
            int spancount =
                (count >= SKY_SPAN_MAX) ?
                SKY_SPAN_MAX : count;

            count -= spancount;

            fixed16_t  snext, tnext;

            fixed16_t sstep = 0; // keep compiler happy
            fixed16_t tstep = 0; // ditto

            if (count) {
                u += spancount;

                // calculate s and t at far end of span,
                // calculate s and t steps across span by shifting
                D_Sky_uv_To_st(u, v, &snext, &tnext);

                sstep = (snext - s) >> SKY_SPAN_SHIFT;
                tstep = (tnext - t) >> SKY_SPAN_SHIFT;
            }
            else {
                // calculate s and t at last pixel in span,
                // calculate s and t steps across span by division
                int spancountminus1 = (float)(spancount - 1);

                if (spancountminus1 > 0) {
                    u += spancountminus1;
                    D_Sky_uv_To_st(u, v, &snext, &tnext);

                    sstep = (snext - s) / spancountminus1;
                    tstep = (tnext - t) / spancountminus1;
                }
            }

            do {
                *pdest++ = r_skysource[FIXED8_TO_INT(t & R_SKY_TMASK) + FIXED16_TO_INT(s & R_SKY_SMASK)];
                s += sstep;
                t += tstep;
            } while (--spancount > 0);

            s = snext;
            t = tnext;

        } while (count > 0);

    } while ((pspan = pspan->pnext) != NULL);
}

