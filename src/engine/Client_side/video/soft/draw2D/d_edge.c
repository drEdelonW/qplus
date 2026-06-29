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
// d_edge.c

#include "q_tools.h"
#include "d_local.h"
#include "r_local.h"
#include "client.h"
#include "Surface.h"

static int _miplevel;   // TODO: make mipLevel enum
float scale_for_mip;


vec3_t  transformed_modelorg;

/*
==============
D_DrawPoly

==============
*/
void D_DrawPoly() {} // this driver takes spans, not polygons


/*
=============
D_MipLevelForScale
=============
*/
int D_MipLevelForScale(float scale) {
    int lMipLevel;
    /**/ if (scale >= d_scalemip[0])    lMipLevel = 0;
    else if (scale >= d_scalemip[1])    lMipLevel = 1;
    else if (scale >= d_scalemip[2])    lMipLevel = 2;
    else /*                       */    lMipLevel = 3;

    CLAMP_MIN(lMipLevel, d_minmip);

    return lMipLevel;
}


/*
==============
D_DrawSolidSurface
==============
*/

// FIXME: clean this up

void D_DrawSolidSurface(Surf_p surf, int color) {
    int32_t pix =
        (color << 24) |
        (color << 16) |
        (color << 8) |
        (color << 0);

    for (eSpan_p span = surf->spans; span; span = span->pnext) {
        uint8_p pdest = (uint8_p)d_viewbuffer + (screenwidth * span->v);
        int u = span->u;
        int u2 = span->u + span->count - 1;
        ((uint8_p)pdest)[u] = pix;

        if (u2 - u < 8) {
            for (u++; u <= u2; u++)
                ((uint8_p)pdest)[u] = pix;
        }
        else {
            for (u++; u & 3; u++)
                ((uint8_p)pdest)[u] = pix;

            u2 -= 4;
            for (; u <= u2; u += 4)
                *(int*)((uint8_p)pdest + u) = pix;
            u2 += 4;
            for (; u <= u2; u++)
                ((uint8_p)pdest)[u] = pix;
        }
    }
}


/*
==============
D_CalcGradients
==============
*/
void D_CalcGradients(mSurface_p pface) {
    // mPlane_p  pplane = pface->plane;

    float mipscale = 1.0 / (float)(1 << _miplevel);

    vec3_t p_saxis = TransformVector(*(vec3_p)(&pface->texinfo->vecs[S_AX]));
    vec3_t p_taxis = TransformVector(*(vec3_p)(&pface->texinfo->vecs[T_AX]));
    {
        float s = xscaleinv * mipscale;
        d_sdivzstepu = p_saxis.v[S_AX] * s;
        d_tdivzstepu = p_taxis.v[S_AX] * s;
    }
    {
        float t = yscaleinv * mipscale;
        d_sdivzstepv = -p_saxis.v[T_AX] * t;
        d_tdivzstepv = -p_taxis.v[T_AX] * t;
    }
    d_sdivzorigin =
        mipscale * p_saxis.z +
        -xcenter * d_sdivzstepu +
        -ycenter * d_sdivzstepv;
    d_tdivzorigin =
        mipscale * p_taxis.z +
        -xcenter * d_tdivzstepu +
        -ycenter * d_tdivzstepv;

    vec3_t p_temp1 = VectorScale(transformed_modelorg, mipscale);

    {
        float t = FIXED16_ONE * mipscale;
        sadjust =
            ((fixed16_t)(DotProduct(p_temp1, p_saxis) * FIXED16_ONE + 0.5f)) -
            (INT_TO_FIXED16(pface->texturemins[S_AX]) >> _miplevel) +
            (pface->texinfo->vecs[S_AX].offs * t);
        tadjust =
            ((fixed16_t)(DotProduct(p_temp1, p_taxis) * FIXED16_ONE + 0.5f)) -
            (INT_TO_FIXED16(pface->texturemins[T_AX]) >> _miplevel) +
            pface->texinfo->vecs[T_AX].offs * t;
    }
    //
    // -1 (-epsilon) so we never wander off the edge of the texture
    //
    bbextents = (INT_TO_FIXED16(pface->extents[S_AX]) >> _miplevel) - 1;
    bbextentt = (INT_TO_FIXED16(pface->extents[T_AX]) >> _miplevel) - 1;
}


/*
==============
D_DrawSurfaces
==============
*/
void D_DrawSurfaces() {
    currententity = &cl_entities[0];
    transformed_modelorg = TransformVector(modelorg);
    vec3_t world_transformed_modelorg = transformed_modelorg;

    // TODO: could preset a lot of this at mode set time
    if (r_drawflat.value) {
        for (Surf_p surf = &surfaces[1]; surf < surface_p; surf++) {
            if (!surf->spans)       continue;

            d_zistepu = surf->d_zistepu;
            d_zistepv = surf->d_zistepv;
            d_ziorigin = surf->d_ziorigin;

            // D_DrawSolidSurface(surf, (int)surf->data & 0xFF);
            D_DrawSolidSurface(surf, (int)((uintptr_t)surf->data & 0xFF));
            D_DrawZSpans(surf->spans);
        }
    }
    else {
        for (Surf_p surf = &surfaces[1]; surf < surface_p; surf++) {
            if (!surf->spans)       continue;

            r_drawnpolycount++;

            d_zistepu = surf->d_zistepu;
            d_zistepv = surf->d_zistepv;
            d_ziorigin = surf->d_ziorigin;

            if (surf->flags & SURF_DRAWSKY) {
                if (!r_skymade)
                    R_MakeSky();

                D_DrawSkyScans8(surf->spans);
                D_DrawZSpans(surf->spans);
            }
            else if (surf->flags & SURF_DRAWBACKGROUND) {
                // set up a gradient for the background surface that places it
                // effectively at infinity distance from the viewpoint
                d_zistepu = 0.0f;
                d_zistepv = 0.0f;
                d_ziorigin = -0.9f;

                D_DrawSolidSurface(surf, (int)r_clearcolor.value & 0xFF);
                D_DrawZSpans(surf->spans);
            }
            else if (surf->flags & SURF_DRAWTURB) {
                mSurface_p pface = surf->data;
                _miplevel = 0;
                cacheblock = (pixel_p)(
                    (uint8_p)pface->texinfo->texture +
                    pface->texinfo->texture->offsets[0]
                    );
                cachewidth = 64;

                if (surf->insubmodel) {
                    // FIXME: we don't want to do all this for every polygon!
                    // TODO: store once at start of frame
                    currententity = surf->entity; //FIXME: make this passed in to
                    // R_RotateBmodel()
                    vec3_t local_modelorg = VectorSubtract(r_origin, currententity->origin);
                    transformed_modelorg = TransformVector(local_modelorg);

                    R_RotateBmodel(); // FIXME: don't mess with the frustum,
                    // make entity passed in
                }

                D_CalcGradients(pface);
                Turbulent8(surf->spans);
                D_DrawZSpans(surf->spans);

                if (surf->insubmodel) {
                    //
                    // restore the old drawing state
                    // FIXME: we don't want to do this every time!
                    // TODO: speed up
                    //
                    currententity = &cl_entities[0];
                    transformed_modelorg = world_transformed_modelorg;
                    BS = base_BS;
                    modelorg = base_modelorg;
                    R_TransformFrustum();
                }
            }
            else {
                if (surf->insubmodel) {
                    // FIXME: we don't want to do all this for every polygon!
                    // TODO: store once at start of frame
                    currententity = surf->entity; //FIXME: make this passed in to
                    // R_RotateBmodel()
                    vec3_t local_modelorg = VectorSubtract(r_origin, currententity->origin);
                    transformed_modelorg = TransformVector(local_modelorg);

                    R_RotateBmodel(); // FIXME: don't mess with the frustum,
                    // make entity passed in
                }

                mSurface_p pface = surf->data;
                _miplevel = D_MipLevelForScale(
                    surf->nearzi * scale_for_mip *
                    pface->texinfo->mipadjust
                );

                // FIXME: make this passed in to D_CacheSurface
                SurfCache_p pcurrentcache = D_CacheSurface(pface, _miplevel);

                cacheblock = (pixel_p)pcurrentcache->data;
                cachewidth = pcurrentcache->width;

                D_CalcGradients(pface);

                (*d_drawspans)(surf->spans);

                D_DrawZSpans(surf->spans);

                if (surf->insubmodel) {
                    //
                    // restore the old drawing state
                    // FIXME: we don't want to do this every time!
                    // TODO: speed up
                    //
                    currententity = &cl_entities[0];
                    transformed_modelorg = world_transformed_modelorg;
                    BS = base_BS;
                    modelorg = base_modelorg;
                    R_TransformFrustum();
                }
            }
        }
    }
}

