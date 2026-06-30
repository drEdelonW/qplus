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

uint8_p r_skysource;

bool r_skymade;
// TODO: clean up these routines

static uint8_t bottomsky[128 * 131];
static uint8_t bottommask[128 * 131];
static uint8_t newsky[128 * 256];
// newsky and topsky both pack in here,
// 128 bytes of newsky on the left of each scan,
// 128 bytes of topsky on the right,
// because the low-level drawers need 256-uint8_t scan widths


/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky(Texture_p mt) {
    uint8_p src = (uint8_p)mt + mt->offsets[0];

    for (int i = 0; i < 128; i++)
        for (int j = 0; j < 128; j++)
            newsky[MUL256(i) + j + 128] = src[MUL256(i) + j + 128];

    for (int i = 0; i < 128; i++)
        for (int j = 0; j < 131; j++)
            if (src[MUL256(i) + (j & 0x7F)]) {
                bottomsky[(i * 131) + j] = src[MUL256(i) + (j & 0x7F)];
                bottommask[(i * 131) + j] = 0x00;
            }
            else {
                bottomsky[(i * 131) + j] = 0x00;
                bottommask[(i * 131) + j] = 0xFF;
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
    uint8_p pnewsky = &newsky[0];
#endif

    for (int y = 0; y < SKYSIZE; y++) {
        int baseofs = ((y + yshift) & SKYMASK) * 131;

#if UNALIGNED_OK
        for (int x = 0; x < SKYSIZE; x += 4) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);

            *pnewsky = (
                *(pnewsky + (128 / sizeof(uint32_t))) &
                *(uint32_p)&bottommask[ofs]) |
                *(uint32_p)&bottomsky[ofs];
            pnewsky++;
        }
#else
        for (int x = 0; x < SKYSIZE; x++) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);

            *pnewsky = (
                *(pnewsky + 128) &
                *(uint8_p)&bottommask[ofs]) |
                *(uint8_p)&bottomsky[ofs];
            pnewsky++;
        }
#endif

        pnewsky += 128 / sizeof(*pnewsky);
    }

    r_skymade = true;
}


/*
=================
R_GenSkyTile
=================
*/
void R_GenSkyTile(uint8_p pdest) {
    int xshift = skytime * skyspeed;
    int yshift = skytime * skyspeed;

#if UNALIGNED_OK
    uint32_p pnewsky = (uint32_p)&newsky[0];
    uint32_p pd = (uint32_p)pdest;
#else
    uint8_p pnewsky = &newsky[0];
    uint8_p pd = pdest;
#endif

    for (int y = 0; y < SKYSIZE; y++) {
        int baseofs = ((y + yshift) & SKYMASK) * 131;

#if UNALIGNED_OK
        for (int x = 0; x < SKYSIZE; x += 4) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);

            *pd = (
                *(pnewsky + (128 / sizeof(uint32_t))) &
                *(uint32_p)&bottommask[ofs]
                ) |
                *(uint32_p)&bottomsky[ofs];
            pnewsky++;
            pd++;
        }
#else
        for (int x = 0; x < SKYSIZE; x++) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);

            *pd = (
                *(pnewsky + 128) &
                *(uint8_p)&bottommask[ofs]
                ) |
                *(uint8_p)&bottomsky[ofs];
            pnewsky++;
            pd++;
        }
#endif

        pnewsky += 128 / sizeof(*pnewsky);
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

    uint8_p pnewsky = &newsky[0];
    uint16_p pd = pdest;

    for (int y = 0; y < SKYSIZE; y++) {
        int baseofs = ((y + yshift) & SKYMASK) * 131;

        // FIXME: clean this up
        // FIXME: do faster unaligned version?
        for (int x = 0; x < SKYSIZE; x++) {
            int ofs = baseofs + ((x + xshift) & SKYMASK);

            *pd = d_8to16table[
                (
                    *(pnewsky + 128) &
                    *(uint8_p)&bottommask[ofs]
                    ) |
                    *(uint8_p)&bottomsky[ofs]
            ];
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

    skytime = cl.time - ((int)(cl.time / temp) * temp);

    r_skymade = false;
}


