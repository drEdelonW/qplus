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
#include "mem_placement.h"
#include "host.h"
#include "endian_tools.h"
#include "d_local.h"
#include "render.h"

VidDef_t vid;    // global video state

#define BASEWIDTH  320
#define BASEHEIGHT 200

static uint8_t _vidBuf[BASEWIDTH * BASEHEIGHT] PLACE_TO_SDRAM;
static int16_t _zBuf[BASEWIDTH * BASEHEIGHT] PLACE_TO_SDRAM;
static uint8_t _surfCache[256 * 1024] PLACE_TO_SDRAM;

uint16_t d_8to16table[256];
uint32_t d_8to24table[256];

__weak void VID_SetPalette(uint8_p palette) {}
__weak void VID_ShiftPalette(uint8_p palette) {}
__weak void D_BeginDirectRect(int x, int y, uint8_p pbitmap, int width, int height) {}
__weak void D_EndDirectRect(int x, int y, int width, int height) {}
__weak void VID_Update(vRect_p rects) {}

__weak void VID_Shutdown() {}
__weak void VID_Init(uint8_p palette) {
    vid = (VidDef_t){
        .buffer         = _vidBuf,
        .colormap       = host_colormap,
        // .colormap16
        .fullbright     = 256 - LittleLong(*((int*)host_colormap + 2048)),
        .rowbytes       = BASEWIDTH,
        .width          = BASEWIDTH,
        .height         = BASEHEIGHT,
        .aspect         = 1.0,
        .numpages       = 1,
        // .recalc_refdef
        .conbuffer      = _vidBuf,
        .conrowbytes    = BASEWIDTH,
        .conwidth       = BASEWIDTH,
        .conheight      = BASEHEIGHT,
        .maxwarpwidth   = BASEWIDTH,
        .maxwarpheight  = BASEHEIGHT,
        // .direct
    };

    d_pzbuffer = _zBuf;
    D_InitCaches(_surfCache, sizeof(_surfCache));
}


