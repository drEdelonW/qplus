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

#include "quakedef.h"
#include "r_local.h"

drawsurf_t r_drawsurf;

int    lightleft, sourcesstep, blocksize, sourcetstep;
int    lightdelta, lightdeltastep;
int    lightright, lightleftstep, lightrightstep, blockdivshift;
uint32_t  blockdivmask;
typeless_ptr prowdestbase;
uint8_p pbasesource;
int    surfrowbytes; // used by ASM files
uint32_p r_lightptr;
int    r_stepback;
int    r_lightwidth;
int    r_numhblocks, r_numvblocks;
uint8_p r_source;
uint8_p r_sourcemax;

void R_DrawSurfaceBlock8_mip0();
void R_DrawSurfaceBlock8_mip1();
void R_DrawSurfaceBlock8_mip2();
void R_DrawSurfaceBlock8_mip3();

static void (*surfmiptable[4])() = {
    R_DrawSurfaceBlock8_mip0,
    R_DrawSurfaceBlock8_mip1,
    R_DrawSurfaceBlock8_mip2,
    R_DrawSurfaceBlock8_mip3
};



uint32_t  blocklights[18 * 18];

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights() {
    msurface_p surf = r_drawsurf.surf;
    int smax = (surf->extents[0] >> 4) + 1;
    int tmax = (surf->extents[1] >> 4) + 1;
    mtexinfo_p tex = surf->texinfo;

    for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
        if (!(surf->dlightbits & (1 << lnum)))
            continue;  // not lit by this light

        float rad = cl_dlights[lnum].radius;
        float dist = DotProduct(cl_dlights[lnum].origin, surf->plane->normal) -
            surf->plane->dist;
        rad -= fabs(dist);
        float minlight = cl_dlights[lnum].minlight;
        if (rad < minlight)
            continue;
        minlight = rad - minlight;

        vec3_t impact;
        for (int i = 0; i < VECT_DIM; i++) {
            impact[i] = cl_dlights[lnum].origin[i] -
                surf->plane->normal[i] * dist;
        }

        vec3_t   local;
        local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
        local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

        local[0] -= surf->texturemins[0];
        local[1] -= surf->texturemins[1];

        for (int t = 0; t < tmax; t++) {
            int td = local[1] - t * 16;
            if (td < 0)
                td = -td;
            for (int s = 0; s < smax; s++) {
                int sd = local[0] - s * 16;
                if (sd < 0)
                    sd = -sd;
                if (sd > td)
                    dist = sd + (td >> 1);
                else
                    dist = td + (sd >> 1);
                if (dist < minlight)
#ifdef QUAKE2
                {
                    uint32_t temp;
                    temp = (rad - dist) * 256;
                    i = t * smax + s;
                    if (!cl_dlights[lnum].dark)
                        blocklights[i] += temp;
                    else {
                        if (blocklights[i] > temp)
                            blocklights[i] -= temp;
                        else
                            blocklights[i] = 0;
                    }
                }
#else
                    blocklights[t * smax + s] += (rad - dist) * 256;
#endif
            }
        }
    }
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap() {
    // return;
    msurface_p surf = r_drawsurf.surf;
    int smax = (surf->extents[0] >> 4) + 1;
    int tmax = (surf->extents[1] >> 4) + 1;
    int size = smax * tmax;
    uint8_p lightmap = surf->samples;

    if (r_fullbright.value || !cl.worldmodel->lightdata) {
        for (int i = 0; i < size; i++)
            blocklights[i] = 0;
        return;
    }

    // clear to ambient
    for (int i = 0; i < size; i++)
        blocklights[i] = r_refdef.ambientlight << 8;


    // add all the lightmaps
    if (lightmap)
        for (int maps = 0; (maps < MAXLIGHTMAPS) && (surf->styles[maps] != 255); maps++) {
            uint32_t scale = r_drawsurf.lightadj[maps]; // 8.8 fraction
            for (int i = 0; i < size; i++)
                blocklights[i] += lightmap[i] * scale;
            lightmap += size; // skip to next lightmap
        }

    // add all the dynamic lights
    if (surf->dlightframe == r_framecount)
        R_AddDynamicLights();

    // bound, invert, and shift
    for (int i = 0; i < size; i++) {
        int t = (255 * 256 - (int)blocklights[i]) >> (8 - VID_CBITS);

        if (t < (1 << 6))
            t = (1 << 6);

        blocklights[i] = t;
    }
}


/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_p R_TextureAnimation(texture_p base) {
    if ((currententity->frame) &&
        (base->alternate_anims))
        base = base->alternate_anims;

    if (!base->anim_total)
        return base;

    int reletive = (int)(cl.time * 10) % base->anim_total;

    int count = 0;
    while ((base->anim_min > reletive) ||
        (base->anim_max <= reletive)) {
        base = base->anim_next;
        if (!base)
            Sys_Error("R_TextureAnimation: broken cycle");
        if (++count > 100)
            Sys_Error("R_TextureAnimation: infinite cycle");
    }

    return base;
}


/*
===============
R_DrawSurface
===============
*/
void R_DrawSurface() {
    void (*pblockdrawer)();
    // calculate the lightings
    R_BuildLightMap();

    surfrowbytes = r_drawsurf.rowbytes;
    texture_p mt = r_drawsurf.texture;
    r_source = (uint8_p)mt + mt->offsets[r_drawsurf.surfmip];

    // the fractional light values should range from 0 to (VID_GRADES - 1) << 16
    // from a source range of 0 - 255

    int texwidth = mt->width >> r_drawsurf.surfmip;

    blocksize = 16 >> r_drawsurf.surfmip;
    blockdivshift = 4 - r_drawsurf.surfmip;
    blockdivmask = (1 << blockdivshift) - 1;

    r_lightwidth = (r_drawsurf.surf->extents[0] >> 4) + 1;
    r_numhblocks = r_drawsurf.surfwidth >> blockdivshift;
    r_numvblocks = r_drawsurf.surfheight >> blockdivshift;

    //==============================

    int horzblockstep;
    if (r_pixbytes == 1) {
        pblockdrawer = surfmiptable[r_drawsurf.surfmip];
        // TODO: only needs to be set when there is a display settings change
        horzblockstep = blocksize;
    }
    else {
        pblockdrawer = R_DrawSurfaceBlock16;
        // TODO: only needs to be set when there is a display settings change
        horzblockstep = blocksize << 1;
    }

    int smax = mt->width >> r_drawsurf.surfmip;
    int twidth = texwidth;
    int tmax = mt->height >> r_drawsurf.surfmip;
    sourcetstep = texwidth;
    r_stepback = tmax * twidth;
    r_sourcemax = r_source + (tmax * smax);
    int soffset = r_drawsurf.surf->texturemins[0];
    int basetoffset = r_drawsurf.surf->texturemins[1];

    // << 16 components are to guarantee positive values for %
    soffset = ((soffset >> r_drawsurf.surfmip) + (smax << 16)) % smax;
    uint8_p basetptr = &r_source[(
        (((basetoffset >> r_drawsurf.surfmip) +
            (tmax << 16)) % tmax) *
        twidth)];

    uint8_p pcolumndest = r_drawsurf.surfdat;
    for (int u = 0; u < r_numhblocks; u++) {
        r_lightptr = blocklights + u;
        prowdestbase = pcolumndest;
        pbasesource = basetptr + soffset;
        (*pblockdrawer)();
        soffset = soffset + blocksize;
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
    uint8_p psource = pbasesource;
    uint8_p prowdest = prowdestbase;

    for (int v = 0; v < r_numvblocks; v++) {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        lightleft = r_lightptr[0];
        lightright = r_lightptr[1];
        r_lightptr += r_lightwidth;
        lightleftstep = (r_lightptr[0] - lightleft) >> 4;
        lightrightstep = (r_lightptr[1] - lightright) >> 4;

        for (int i = 0; i < 16; i++) {
            int lighttemp = lightleft - lightright;
            int lightstep = lighttemp >> 4;
            int light = lightright;

            for (int b = 15; b >= 0; b--) {
                uint8_t pix = psource[b];
                prowdest[b] = ((uint8_p)vid.colormap)
                    [(light & 0xFF00) + pix];
                light += lightstep;
            }

            psource += sourcetstep;
            lightright += lightrightstep;
            lightleft += lightleftstep;
            prowdest += surfrowbytes;
        }

        if (psource >= r_sourcemax)
            psource -= r_stepback;
    }
}


/*
================
R_DrawSurfaceBlock8_mip1
================
*/
void R_DrawSurfaceBlock8_mip1() {
    uint8_p psource = pbasesource;
    uint8_p prowdest = prowdestbase;

    for (int v = 0; v < r_numvblocks; v++) {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        lightleft = r_lightptr[0];
        lightright = r_lightptr[1];
        r_lightptr += r_lightwidth;
        lightleftstep = (r_lightptr[0] - lightleft) >> 3;
        lightrightstep = (r_lightptr[1] - lightright) >> 3;

        for (int i = 0; i < 8; i++) {
            int lighttemp = lightleft - lightright;
            int lightstep = lighttemp >> 3;
            int light = lightright;

            for (int b = 7; b >= 0; b--) {
                uint8_t pix = psource[b];
                prowdest[b] = ((uint8_p)vid.colormap)
                    [(light & 0xFF00) + pix];
                light += lightstep;
            }

            psource += sourcetstep;
            lightright += lightrightstep;
            lightleft += lightleftstep;
            prowdest += surfrowbytes;
        }

        if (psource >= r_sourcemax)
            psource -= r_stepback;
    }
}


/*
================
R_DrawSurfaceBlock8_mip2
================
*/
void R_DrawSurfaceBlock8_mip2() {
    uint8_p psource = pbasesource;
    uint8_p prowdest = prowdestbase;

    for (int v = 0; v < r_numvblocks; v++) {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        lightleft = r_lightptr[0];
        lightright = r_lightptr[1];
        r_lightptr += r_lightwidth;
        lightleftstep = (r_lightptr[0] - lightleft) >> 2;
        lightrightstep = (r_lightptr[1] - lightright) >> 2;

        for (int i = 0; i < 4; i++) {
            int lighttemp = lightleft - lightright;
            int lightstep = lighttemp >> 2;

            int light = lightright;

            for (int b = 3; b >= 0; b--) {
                uint8_t pix = psource[b];
                prowdest[b] = ((uint8_p)vid.colormap)
                    [(light & 0xFF00) + pix];
                light += lightstep;
            }

            psource += sourcetstep;
            lightright += lightrightstep;
            lightleft += lightleftstep;
            prowdest += surfrowbytes;
        }

        if (psource >= r_sourcemax)
            psource -= r_stepback;
    }
}


/*
================
R_DrawSurfaceBlock8_mip3
================
*/
void R_DrawSurfaceBlock8_mip3() {
    uint8_p psource = pbasesource;
    uint8_p prowdest = prowdestbase;

    for (int v = 0; v < r_numvblocks; v++) {
        // FIXME: make these locals?
        // FIXME: use delta rather than both right and left, like ASM?
        lightleft = r_lightptr[0];
        lightright = r_lightptr[1];
        r_lightptr += r_lightwidth;
        lightleftstep = (r_lightptr[0] - lightleft) >> 1;
        lightrightstep = (r_lightptr[1] - lightright) >> 1;

        for (int i = 0; i < 2; i++) {
            int lighttemp = lightleft - lightright;
            int lightstep = lighttemp >> 1;

            int light = lightright;

            for (int b = 1; b >= 0; b--) {
                uint8_t pix = psource[b];
                prowdest[b] = ((uint8_p)vid.colormap)
                    [(light & 0xFF00) + pix];
                light += lightstep;
            }

            psource += sourcetstep;
            lightright += lightrightstep;
            lightleft += lightleftstep;
            prowdest += surfrowbytes;
        }

        if (psource >= r_sourcemax)
            psource -= r_stepback;
    }
}


/*
================
R_DrawSurfaceBlock16

FIXME: make this work
================
*/
void R_DrawSurfaceBlock16() {
    uint16_p prowdest = (uint16_p)prowdestbase;

    for (int k = 0; k < blocksize; k++) {
        uint8_p psource = pbasesource;
        int lighttemp = lightright - lightleft;
        int lightstep = lighttemp >> blockdivshift;

        int light = lightleft;
        uint16_p pdest = prowdest;

        for (int b = 0; b < blocksize; b++) {
            uint8_t pix = *psource;
            *pdest = vid.colormap16[(light & 0xFF00) + pix];
            psource += sourcesstep;
            pdest++;
            light += lightstep;
        }

        pbasesource += sourcetstep;
        lightright += lightrightstep;
        lightleft += lightleftstep;
        prowdest = (uint16_p)((uint8_p)prowdest + surfrowbytes);
    }

    prowdestbase = prowdest;
}

#endif


//============================================================================

/*
================
R_GenTurbTile
================
*/
void R_GenTurbTile(pixel_p pbasetex, typeless_ptr pdest) {
    int* turb = sintable + ((int)(cl.time * SPEED) & (CYCLE - 1));
    uint8_p pd = (uint8_p)pdest;

    for (int i = 0; i < TILE_SIZE; i++) {
        for (int j = 0; j < TILE_SIZE; j++) {
            int s = (((j << 16) + turb[i & (CYCLE - 1)]) >> 16) & 63;
            int t = (((i << 16) + turb[j & (CYCLE - 1)]) >> 16) & 63;
            *pd++ = *(pbasetex + (t << 6) + s);
        }
    }
}


/*
================
R_GenTurbTile16
================
*/
void R_GenTurbTile16(pixel_p pbasetex, typeless_ptr pdest) {
    int* turb = sintable + ((int)(cl.time * SPEED) & (CYCLE - 1));
    uint16_p pd = (uint16_p)pdest;

    for (int i = 0; i < TILE_SIZE; i++) {
        for (int j = 0; j < TILE_SIZE; j++) {
            int s = (((j << 16) + turb[i & (CYCLE - 1)]) >> 16) & 63;
            int t = (((i << 16) + turb[j & (CYCLE - 1)]) >> 16) & 63;
            *pd++ = d_8to16table[*(pbasetex + (t << 6) + s)];
        }
    }
}


/*
================
R_GenTile
================
*/
void R_GenTile(msurface_t* psurf, typeless_ptr pdest) {
    if (psurf->flags & SURF_DRAWTURB) {
        if (r_pixbytes == 1) {
            R_GenTurbTile((pixel_p)
                ((uint8_p)psurf->texinfo->texture + psurf->texinfo->texture->offsets[0]), pdest);
        }
        else {
            R_GenTurbTile16((pixel_p)
                ((uint8_p)psurf->texinfo->texture + psurf->texinfo->texture->offsets[0]), pdest);
        }
    }
    else if (psurf->flags & SURF_DRAWSKY) {
        if (r_pixbytes == 1) {
            R_GenSkyTile(pdest);
        }
        else {
            R_GenSkyTile16(pdest);
        }
    }
    else {
        Sys_Error("Unknown tile type");
    }
}

