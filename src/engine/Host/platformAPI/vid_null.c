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
// vid_null.c -- null video driver to aid porting efforts

#include "vid.h"
#include "mem_placement.h"  // __weak
#ifndef GLQUAKE
#include "d_local.h"    // d_pzbuffer

static qColor8_t _vidBuf[BASEWIDTH * BASEHEIGHT] PLACE_TO_SDRAM;
static int16_t _zBuf[BASEWIDTH * BASEHEIGHT] PLACE_TO_SDRAM;
static uint8_t _surfCache[256 * 1024] PLACE_TO_SDRAM;
#else
static const qColor8_p _vidBuf = NULL;
#endif

Rgb16_t d_8to16table[InksNum];
Rgb24_t d_8to24table[InksNum];
VidDef_t vid = {    // global video state
        .frameBuff = {
            .width      = BASEWIDTH,
            .height     = BASEHEIGHT,
            .pClr       = _vidBuf,
            .rowBytes   = BASEWIDTH,
        },
        .con = {
            .width      = BASEWIDTH,
            .height     = BASEHEIGHT,
            .pClr       = _vidBuf,
            .rowBytes   = BASEWIDTH,
        },
        .maxwarp = {
            .width  = BASEWIDTH,
            .height = BASEHEIGHT
        },

        .direct     = NULL,
        .numpages   = 1,
    };


__weak void VID_SetPalette(qPal_p palette) {}
__weak void VID_ShiftPalette(qPal_p palette) {}
__weak void D_BeginDirectRect(int x, int y, qColor8_p pbitmap, int width, int height) {}
__weak void D_EndDirectRect(int x, int y, int width, int height) {}
__weak void VID_Update(vRect_p rects) {}
__weak void VID_Shutdown() {}
__weak void VID_Init(qPal_p palette) {
#ifndef GLQUAKE
    vid.colormap = host_colormap;

    vid.zBuff.pZBuff = _zBuf;
    D_InitCaches((SurfCache_p)_surfCache, sizeof(_surfCache));
#endif
}


