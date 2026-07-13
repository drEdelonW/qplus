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
// r_surf.c: surface-related refresh code

#include "r_local.h"
#include "render.h"
#include "host.h"
#include "Surface.h"

DrawSurf_t r_drawsurf;  // extern

static int _sourceTstep;
static int _blockSize;
static int _lightLeft;
static int _lightRight;
static int _blockDivShift;
static int _lightLeftStep;
static int  _lightRightStep;
static TypeLess_ptr _pRowDestBase;
static qColor8_p _pBaseSource;
static int _surfRowBytes; // used by ASM files
static int _r_StepBack;
static int _r_LightWidth;
static int _r_NumHBlocks;
static int _r_NumVBlocks;
static qColor8_p _r_Source;
static qColor8_p _r_SourceMax;

void R_DrawSurfaceBlock8_mip0();
void R_DrawSurfaceBlock8_mip1();
void R_DrawSurfaceBlock8_mip2();
void R_DrawSurfaceBlock8_mip3();

static void (*surfmiptable[MIPLEVELS])() = {
    R_DrawSurfaceBlock8_mip0,
    R_DrawSurfaceBlock8_mip1,
    R_DrawSurfaceBlock8_mip2,
    R_DrawSurfaceBlock8_mip3
};

static fixed16_p _r_LightPtr;
static fixed16_t  _blockLights[18 * 18];

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights() { // TODO: merge with GL function almoust the same
    mSurface_p surf = r_drawsurf.surf;

    for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
        if (!(surf->dlightbits & (1 << lnum)))
            continue;   // not lit by this light

        float dist = DotProduct(cl_dlights[lnum].origin, surf->plane->normal) - surf->plane->dist;
        float rad = cl_dlights[lnum].radius - fabs(dist);

        float minlight = cl_dlights[lnum].minlight;
        if (rad < minlight)
            continue;

        minlight = rad - minlight;

        vec3_t impact = VectorMA(
            cl_dlights[lnum].origin,
            -dist, surf->plane->normal
        );
        mTexInfo_p tex = surf->texinfo;
        fixed4_t ts = DotProduct(impact, tex->vecs[S_AX].vx) + tex->vecs[S_AX].offs - surf->texturemins[S_AX];
        fixed4_t tt = DotProduct(impact, tex->vecs[T_AX].vx) + tex->vecs[T_AX].offs - surf->texturemins[T_AX];
        int smax = (FIXED4_TO_INT(surf->extents[S_AX])) + 1;
        int tmax = (FIXED4_TO_INT(surf->extents[T_AX])) + 1;

        for (int t = 0; t < tmax; t++) {
            int td = tt - MUL16(t);
            if (td < 0)     td = -td;

            for (int s = 0; s < smax; s++) {
                int sd = ts - MUL16(s);
                if (sd < 0)     sd = -sd;

                float dist = (sd > td) ?
                    sd + HALF(td) : td + HALF(sd);

                if (dist < minlight)
#ifdef QUAKE2
                {
                    uint32_t temp = (rad - dist) * 256;
                    i = t * smax + s;
                    if (!cl_dlights[lnum].dark)     _blockLights[i] += temp;
                    else {
                        if (_blockLights[i] > temp)  _blockLights[i] -= temp;
                        else                        _blockLights[i] = 0;
                    }
                }
#else
                    _blockLights[(t * smax) + s] += (rad - dist) * 256;
#endif
            }
        }
    }
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in _blockLights
===============
*/
void R_BuildLightMap() {// TODO: merge with GL function almoust the same
    mSurface_p surf = r_drawsurf.surf;
    int smax = FIXED4_TO_INT(surf->extents[S_AX]) + 1;
    int tmax = FIXED4_TO_INT(surf->extents[T_AX]) + 1;
    int size = smax * tmax;


    if ((r_fullbright.value) ||
        (!cl.worldmodel->lightdata)
        ) {
        for (int i = 0; i < size; i++)
            _blockLights[i] = 0;
        return;
    }
    else {  // clear to ambient
        for (int i = 0; i < size; i++)
            _blockLights[i] = INT_TO_FIXED8(r_refdef.ambientLight);

        {   // add all the lightmaps
            uint8_p lightmap = surf->samples;
            if (lightmap)
                for (int maps = 0; (maps < MAXLIGHTMAPS) && (surf->styles[maps] != 255); maps++) {
                    fixed8_t scale = r_drawsurf.lightadj[maps]; // 8.8 fraction
                    for (int i = 0; i < size; i++)
                        _blockLights[i] += lightmap[i] * scale;
                    lightmap += size; // skip to next lightmap
                }
        }
        // add all the dynamic lights
        if (surf->dlightframe == r_framecount)
            R_AddDynamicLights();

        // bound, invert, and shift
    }
    for (int i = 0; i < size; i++) {
        int t = (255 * 256 - (int)_blockLights[i]) >> (8 - VID_CBITS);
        ClampLessThen(&t, 64);

        _blockLights[i] = t;
    }
}




/*
===============
R_DrawSurface
===============
*/
void R_DrawSurface() {
    void (*pblockdrawer)();
    // calculate the lightings
    if (r_lightmap.value) {
        R_BuildLightMap();
    }

    _surfRowBytes = r_drawsurf.rowbytes;
    Texture_p mt = r_drawsurf.texture;
    _r_Source = GetMipPtr(mt, r_drawsurf.surfmip);

    // the fractional light values should range from 0 to INT_TO_FIXED16(VID_GRADES - 1)
    // from a source range of 0 - 255

    int texwidth = mt->width >> r_drawsurf.surfmip;

    _blockSize = 16 >> r_drawsurf.surfmip;
    _blockDivShift = 4 - r_drawsurf.surfmip;
    // blockdivmask = (1 << _blockDivShift) - 1;

    _r_LightWidth = FIXED4_TO_INT(r_drawsurf.surf->extents[0]) + 1;
    _r_NumHBlocks = r_drawsurf.surfwidth >> _blockDivShift;
    _r_NumVBlocks = r_drawsurf.surfheight >> _blockDivShift;

    //==============================

    int horzblockstep;
    if (r_pixbytes == 1) {
        pblockdrawer = surfmiptable[r_drawsurf.surfmip];
        // TODO: only needs to be set when there is a display settings change
        horzblockstep = _blockSize;
    }
    else {
        pblockdrawer = R_DrawSurfaceBlock16;
        // TODO: only needs to be set when there is a display settings change
        horzblockstep = TWICE(_blockSize);
    }

    fixed16_t smax = mt->width >> r_drawsurf.surfmip;
    int twidth = texwidth;
    fixed16_t tmax = mt->height >> r_drawsurf.surfmip;
    _sourceTstep = texwidth;
    _r_StepBack = tmax * twidth;
    _r_SourceMax = _r_Source + (tmax * smax);
    int soffset = r_drawsurf.surf->texturemins[0];
    int basetoffset = r_drawsurf.surf->texturemins[1];

    // << 16 components are to guarantee positive values for %
    soffset = ((soffset >> r_drawsurf.surfmip) + INT_TO_FIXED16(smax)) % smax;
    qColor8_p basetptr = &_r_Source[
        ((((basetoffset >> r_drawsurf.surfmip) +
            INT_TO_FIXED16(tmax)) %
            tmax) *
            twidth)
    ];

    qColor8_p pcolumndest = r_drawsurf.surfdat;
    for (uint8_t u = 0; u < _r_NumHBlocks; u++) {
        _r_LightPtr = _blockLights + u;
        _pRowDestBase = pcolumndest;
        _pBaseSource = basetptr + soffset;
        (*pblockdrawer)();
        soffset = soffset + _blockSize;
        if (soffset >= smax)
            soffset = 0;
        pcolumndest += horzblockstep;
    }
}


//=============================================================================

#if !id386

/*
================
R_DrawSurfaceBlock8_mip0
================
*/
void R_DrawSurfaceBlock8_mip0() {   // nearest surfaces
    qColor8_p psource = _pBaseSource;
    qColor8_p prowdest = _pRowDestBase;

    for (int v = 0; v < _r_NumVBlocks; v++) {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        _lightLeft = _r_LightPtr[0];
        _lightRight = _r_LightPtr[1];
        _r_LightPtr += _r_LightWidth;
        _lightLeftStep = DIV16(_r_LightPtr[0] - _lightLeft);
        _lightRightStep = DIV16(_r_LightPtr[1] - _lightRight);

        for (int i = 0; i < 16; i++) {
            int lightstep = DIV16(_lightLeft - _lightRight);
            int light = _lightRight;

            for (int b = 15; b >= 0; b--) {
                qColor8_t pix = psource[b];
                prowdest[b] = Scr.pColorMapPal->raw[(light & 0xFF00) + pix.i];
                light += lightstep;
            }

            psource += _sourceTstep;
            _lightRight += _lightRightStep;
            _lightLeft += _lightLeftStep;
            prowdest += _surfRowBytes;
        }

        if (psource >= _r_SourceMax)
            psource -= _r_StepBack;
    }
}


/*
================
R_DrawSurfaceBlock8_mip1
================
*/
void R_DrawSurfaceBlock8_mip1() {
    qColor8_p psource = _pBaseSource;
    qColor8_p prowdest = _pRowDestBase;

    for (int v = 0; v < _r_NumVBlocks; v++) {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        _lightLeft = _r_LightPtr[0];
        _lightRight = _r_LightPtr[1];
        _r_LightPtr += _r_LightWidth;
        _lightLeftStep = DIV8(_r_LightPtr[0] - _lightLeft);
        _lightRightStep = DIV8(_r_LightPtr[1] - _lightRight);

        for (int i = 0; i < 8; i++) {
            int lightstep = DIV8(_lightLeft - _lightRight);
            int light = _lightRight;

            for (int b = 7; b >= 0; b--) {
                qColor8_t pix = psource[b];
                prowdest[b] = Scr.pColorMapPal->raw[(light & 0xFF00) + pix.i];
                light += lightstep;
            }

            psource += _sourceTstep;
            _lightRight += _lightRightStep;
            _lightLeft += _lightLeftStep;
            prowdest += _surfRowBytes;
        }

        if (psource >= _r_SourceMax)
            psource -= _r_StepBack;
    }
}


/*
================
R_DrawSurfaceBlock8_mip2
================
*/
void R_DrawSurfaceBlock8_mip2() {
    qColor8_p psource = _pBaseSource;
    qColor8_p prowdest = _pRowDestBase;

    for (int v = 0; v < _r_NumVBlocks; v++) {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        _lightLeft = _r_LightPtr[0];
        _lightRight = _r_LightPtr[1];
        _r_LightPtr += _r_LightWidth;
        _lightLeftStep = DIV4(_r_LightPtr[0] - _lightLeft);
        _lightRightStep = DIV4(_r_LightPtr[1] - _lightRight);

        for (int i = 0; i < 4; i++) {
            int lightstep = DIV4(_lightLeft - _lightRight);
            int light = _lightRight;

            for (int b = 3; b >= 0; b--) {
                qColor8_t pix = psource[b];
                prowdest[b] = Scr.pColorMapPal->raw[(light & 0xFF00) + pix.i];
                light += lightstep;
            }

            psource += _sourceTstep;
            _lightRight += _lightRightStep;
            _lightLeft += _lightLeftStep;
            prowdest += _surfRowBytes;
        }

        if (psource >= _r_SourceMax)
            psource -= _r_StepBack;
    }
}


/*
================
R_DrawSurfaceBlock8_mip3
================
*/
void R_DrawSurfaceBlock8_mip3() {
    qColor8_p psource = _pBaseSource;
    qColor8_p prowdest = _pRowDestBase;

    for (int v = 0; v < _r_NumVBlocks; v++) {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        _lightLeft = _r_LightPtr[0];
        _lightRight = _r_LightPtr[1];
        _r_LightPtr += _r_LightWidth;
        _lightLeftStep = HALF(_r_LightPtr[0] - _lightLeft);
        _lightRightStep = HALF(_r_LightPtr[1] - _lightRight);

        for (int i = 0; i < 2; i++) {
            int lightstep = HALF(_lightLeft - _lightRight);
            int light = _lightRight;

            for (int b = 1; b >= 0; b--) {
                qColor8_t pix = psource[b];
                prowdest[b] = Scr.pColorMapPal->raw[(light & 0xFF00) + pix.i];
                light += lightstep;
            }

            psource += _sourceTstep;
            _lightRight += _lightRightStep;
            _lightLeft += _lightLeftStep;
            prowdest += _surfRowBytes;
        }

        if (psource >= _r_SourceMax)
            psource -= _r_StepBack;
    }
}


/*
================
R_DrawSurfaceBlock16

FIXME: make this work
================
*/
#include "sys.h"    // Sys_Error
void R_DrawSurfaceBlock16() {
    uint16_p prowdest = (uint16_p)_pRowDestBase;
    if (!Scr.pColorMap16)    Sys_Error("Scr.pColorMap16 if NULL\n");

    for (int k = 0; k < _blockSize; k++) {
        qColor8_p psource = _pBaseSource;
        int lightstep = (_lightRight - _lightLeft) >> _blockDivShift;

        int light = _lightLeft;
        uint16_p pdest = prowdest;

        for (int b = 0; b < _blockSize; b++) {
            qColor8_t pix = *psource;
            *pdest = Scr.pColorMap16[(light & 0xFF00) + pix.i].c;
            // psource += sourcesstep;  // TODO: is this correct?
            pdest++;
            light += lightstep;
        }

        _pBaseSource += _sourceTstep;
        _lightRight += _lightRightStep;
        _lightLeft += _lightLeftStep;
        prowdest = (uint16_p)((uint8_p)prowdest + _surfRowBytes);
    }

    _pRowDestBase = prowdest;
}

#endif


//============================================================================

/*
================
R_GenTurbTile
================
*/
void R_GenTurbTile(qColor8_p pbasetex, qColor8_p pdest) {
    int* turb = sintable + ((int)(GetClSimTime() * SPEED) & (CYCLE - 1));
    qColor8_p pd = pdest;

    for (int i = 0; i < TILE_SIZE; i++) {
        for (int j = 0; j < TILE_SIZE; j++) {
            fixed16_t s = (FIXED16_TO_INT((INT_TO_FIXED16(j)) + turb[i & (CYCLE - 1)])) & 0x3F;
            fixed16_t t = (FIXED16_TO_INT((INT_TO_FIXED16(i)) + turb[j & (CYCLE - 1)])) & 0x3F;
            *pd++ = *(pbasetex + MUL64(t) + s);
        }
    }
}


/*
================
R_GenTurbTile16
================
*/
void R_GenTurbTile16(qColor8_p pbasetex, qColor16_p pdest) {
    int* turb = sintable + ((int)(GetClSimTime() * SPEED) & (CYCLE - 1));
    qColor16_p pd = pdest;

    for (int i = 0; i < TILE_SIZE; i++) {
        for (int j = 0; j < TILE_SIZE; j++) {
            fixed16_t s = (FIXED16_TO_INT((INT_TO_FIXED16(j)) + turb[i & (CYCLE - 1)])) & 0x3F;
            fixed16_t t = (FIXED16_TO_INT((INT_TO_FIXED16(i)) + turb[j & (CYCLE - 1)])) & 0x3F;
            (pd++)->c = d_8to16table[(pbasetex + MUL64(t) + s)->i];
        }
    }
}


/*
================
R_GenTile
================
*/
void R_GenTile(mSurface_p psurf, TypeLess_ptr pdest) {
    if (psurf->flags & SURF_DRAWTURB) {
        if (r_pixbytes == 1)    R_GenTurbTile(GetMipPtr(psurf->texinfo->texture, Mip0), pdest);
        else                    R_GenTurbTile16(GetMipPtr(psurf->texinfo->texture, Mip0), pdest);

    }
    else if (psurf->flags & SURF_DRAWSKY) {
        if (r_pixbytes == 1)    R_GenSkyTile(pdest);
        else                    R_GenSkyTile16(pdest);
    }
    else    Host_SysError("Unknown tile type");
}

