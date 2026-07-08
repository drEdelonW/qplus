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
// r_sky.c

#include "r_local.h"
#include "d_local.h"

static int _iSkySpeed = 8;
static int _iSkySpeed2 = 2;
float skyspeed;
float skyspeed2;
LegDt_t   skytime;

qColor8_p r_skysource;

bool r_skymade;
// TODO: clean up these routines

// bottomsky/bottommask rows are SKYSIZE+3 wide, padded so (x+xshift)&SKYMASK
// can never run off the end of a dword read in the UNALIGNED_OK path
#define SKY_BOTTOM_PAD      3
#define SKY_BOTTOM_STRIDE   (SKYSIZE + SKY_BOTTOM_PAD)   // 131

#define SKY_MASK_OPAQUE         0x00   // bottomsky texel is visible
#define SKY_MASK_TRANSPARENT    0xFF   // bottomsky texel shows topsky through

static qColor8_t bottomsky[SKYSIZE * SKY_BOTTOM_STRIDE];
static qColor8_t bottommask[SKYSIZE * SKY_BOTTOM_STRIDE];
static qColor8_t newsky[SKYSIZE * (SKYSIZE * 2)];
// newsky and topsky both pack in here,
// SKYSIZE bytes of newsky on the left of each scan,
// SKYSIZE bytes of topsky on the right,
// because the low-level drawers need (SKYSIZE*2)-uint8_t scan widths

// blends one texel: topsky masked by bottommask, or-ed with bottomsky
static inline qColor8_t SkyBlendByte(qColor8_p ptopsky, int ofs) {
    return (qColor8_t){
        .i = (((*(ptopsky + SKYSIZE)).i & bottommask[ofs].i) | bottomsky[ofs].i)
    };
}


#if UNALIGNED_OK
// same blend, SKYSIZE/32 texels at once; requires UNALIGNED_OK (unaligned dword access)
static inline uint32_t SkyBlendDword(uint32_p ptopsky, int ofs) {
    return (*(ptopsky + (SKYSIZE / sizeof(uint32_t))) &
        *(uint32_p)&bottommask[ofs]) |
        *(uint32_p)&bottomsky[ofs];
}
#endif

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky(Texture_p mt) {
    qColor8_p src = GetMipPtr(mt, Mip0);

    for (int i = 0; i < SKYSIZE; i++)
        for (int j = 0; j < SKYSIZE; j++)
            newsky[MUL256(i) + j + SKYSIZE] = src[MUL256(i) + j + SKYSIZE];

    for (int i = 0; i < SKYSIZE; i++)
        for (int j = 0; j < SKY_BOTTOM_STRIDE; j++)
            if (src[MUL256(i) + (j & SKYMASK)].i) {
                bottomsky[(i * SKY_BOTTOM_STRIDE) + j] = src[MUL256(i) + (j & SKYMASK)];
                bottommask[(i * SKY_BOTTOM_STRIDE) + j].i = SKY_MASK_OPAQUE;
            }
            else {
                bottomsky[(i * SKY_BOTTOM_STRIDE) + j].i = SKY_MASK_OPAQUE;
                bottommask[(i * SKY_BOTTOM_STRIDE) + j].i = SKY_MASK_TRANSPARENT;
            }


    r_skysource = newsky;
}


/*
=================
R_MakeSky
=================
*/
void R_MakeSky() {
    static int xlast = -1;
    static int ylast = -1;

    int xshift = skytime * skyspeed;
    int yshift = skytime * skyspeed;

    if ((xshift == xlast) &&
        (yshift == ylast)
        ) return;

    xlast = xshift;
    ylast = yshift;

    // PORT: dword path needs UNALIGNED_OK, byte path works everywhere
#if UNALIGNED_OK
    uint32_p pnewsky = (uint32_p)&newsky[0];
#else
    qColor8_p pnewsky = &newsky[0];
#endif

    for (int y = 0; y < SKYSIZE; y++) {
        int baseofs = ((y + yshift) & SKYMASK) * SKY_BOTTOM_STRIDE;

#if UNALIGNED_OK
        for (int x = 0; x < SKYSIZE; x += sizeof(uint32_t)) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);
            *pnewsky = SkyBlendDword(pnewsky, ofs);
            pnewsky++;
        }
#else
        for (int x = 0; x < SKYSIZE; x++) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);
            *pnewsky = SkyBlendByte(pnewsky, ofs);
            pnewsky++;
        }
#endif

        pnewsky += SKYSIZE / sizeof(*pnewsky);
    }

    r_skymade = true;
}


/*
=================
R_GenSkyTile
=================
*/
void R_GenSkyTile(qColor8_p pdest) {
    int xshift = skytime * skyspeed;
    int yshift = skytime * skyspeed;

#if UNALIGNED_OK
    uint32_p pnewsky = (uint32_p)&newsky[0];
    uint32_p pd = (uint32_p)pdest;
#else
    qColor8_p pnewsky = &newsky[0];
    qColor8_p pd = pdest;
#endif

    for (int y = 0; y < SKYSIZE; y++) {
        int baseofs = ((y + yshift) & SKYMASK) * SKY_BOTTOM_STRIDE;

#if UNALIGNED_OK
        for (int x = 0; x < SKYSIZE; x += sizeof(uint32_t)) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);
            *pd = SkyBlendDword(pnewsky, ofs);
            pnewsky++;
            pd++;
        }
#else
        for (int x = 0; x < SKYSIZE; x++) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);
            *pd = SkyBlendByte(pnewsky, ofs);
            pnewsky++;
            pd++;
        }
#endif

        pnewsky += SKYSIZE / sizeof(*pnewsky);
    }
}


/*
=================
R_GenSkyTile16
=================
*/
void R_GenSkyTile16(uint16_p pdest) {
    int xshift = skytime * skyspeed;
    int yshift = skytime * skyspeed;

    qColor8_p pnewsky = &newsky[0];
    uint16_p pd = pdest;

    for (int y = 0; y < SKYSIZE; y++) {
        int baseofs = ((y + yshift) & SKYMASK) * SKY_BOTTOM_STRIDE;

        // FIXME: do faster unaligned version?
        for (int x = 0; x < SKYSIZE; x++) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);
            *pd = d_8to16table[SkyBlendByte(pnewsky, ofs).i];
            pnewsky++;
            pd++;
        }

        pnewsky += TILE_SIZE;
    }
}


/*
=============
R_SetSkyFrame
==============
*/
void R_SetSkyFrame() {
    skyspeed = _iSkySpeed;
    skyspeed2 = _iSkySpeed2;

    int g = GreatestCommonDivisor(_iSkySpeed, _iSkySpeed2);
    int s1 = _iSkySpeed / g;
    int s2 = _iSkySpeed2 / g;
    float temp = SKYSIZE * s1 * s2;

    skytime = GetClSimTime() - ((int)(GetClSimTime() / temp) * temp);

    r_skymade = false;
}


