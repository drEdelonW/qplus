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
// r_light.c

#ifdef GLQUAKE
# include "qOpenGL.h"
# include "client.h"
# include "model.h"
#else
# include "r_local.h"
# include "render.h"
# include "Surface.h"
#endif

int r_dlightframecount;

/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight() {
    // light animations
    // 'm' is normal light, 'a' is no light, 'z' is double bright
    int i = (int)(GetClSimTime() * 10);
    for (int j = 0; j < MAX_LIGHTSTYLES; j++) {
        if (!cl_lightstyle[j].length) {
            d_lightstylevalue[j] = 256;
            continue;
        }
        int k = i % cl_lightstyle[j].length;
        k = cl_lightstyle[j].map[k] - 'a';
        k = k * 22;
        d_lightstylevalue[j] = k;
    }
}


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

void R_MarkLights(dLight_p light, int bit, mNode_p node) {
    if (NodeKind(node) == isLeaf)     return;

    mPlane_p splitplane = node->plane;
    float dist = DotProduct(light->origin, splitplane->normal) - splitplane->dist;

    if (dist > light->radius) {
        R_MarkLights(light, bit, node->children[PsFront]);
        return;
    }
    if (dist < -light->radius) {
        R_MarkLights(light, bit, node->children[PsBack]);
        return;
    }

    // mark the polygons
    mSurface_p surf = cl.worldmodel->surfaces + node->firstsurface;
    for (int i = 0; i < node->numsurfaces; i++, surf++) {
        if (surf->dlightframe != r_dlightframecount) {
            surf->dlightbits = 0;
            surf->dlightframe = r_dlightframecount;
        }
        surf->dlightbits |= bit;
    }

    R_MarkLights(light, bit, node->children[PsFront]);
    R_MarkLights(light, bit, node->children[PsBack]);
}




void R_PushDlights() {
#ifdef GLQUAKE
    if (gl_flashblend.value)    return;
#else
    if (!r_dlightmap.value)     return;
#endif

    r_dlightframecount = r_framecount + 1; // because the count hasn't advanced yet for this frame
    dLight_p l = cl_dlights;

    for (int i = 0; i < MAX_DLIGHTS; i++, l++) {
        if ((l->die < GetClSimTime()) ||
            !(l->radius)
            )   continue;
        R_MarkLights(l, 1 << i, cl.worldmodel->nodes);
    }
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/


#ifdef GLQUAKE
mPlane_p    lightplane;
vec3_t      lightspot;
#endif


contents_t RecursiveLightPoint(mNode_p node, vec3_t start, vec3_t end) {
    if (NodeKind(node) == isLeaf)     return CONTENTS_EMPTY;  // didn't hit anything
    
    // calculate mid point
    // FIXME: optimize for axial
    mPlane_p plane = node->plane;
    float front = DotProduct(start, plane->normal) - plane->dist;   bool fSide = (front < 0.f);
    float back = DotProduct(end, plane->normal) - plane->dist;      bool bSide = (back < 0.f);

    if (bSide == fSide)
        return RecursiveLightPoint(node->children[fSide], start, end);

    float frac = front / (front - back);
    // Linear interpolation: mid = start + (end - start) * frac
    vec3_t mid = VectorMA(start, frac, VectorSubtract(end, start));

    // go down front side
    contents_t lp = RecursiveLightPoint(node->children[fSide], start, mid);
    if (lp >= 0)            return lp;              // hit something
    if (bSide == fSide)     return CONTENTS_EMPTY;  // didn't hit anuthing

#ifdef GLQUAKE
    // check for impact on this node
    lightspot = mid;
    lightplane = plane;
#endif

    mSurface_p surf = cl.worldmodel->surfaces + node->firstsurface;
    for (int i = 0; i < node->numsurfaces; i++, surf++) {
        if (surf->flags & SURF_DRAWTILED)
            continue;   // no lightmaps

        mTexInfo_p tex = surf->texinfo;

        fixed4_t s = DotProduct(mid, tex->vecs[S_AX].vx) + tex->vecs[S_AX].offs;
        fixed4_t t = DotProduct(mid, tex->vecs[T_AX].vx) + tex->vecs[T_AX].offs;

        if ((s < surf->texturemins[S_AX]) ||
            (t < surf->texturemins[T_AX])
            ) continue;

        fixed4_t tds = s - surf->texturemins[S_AX];
        fixed4_t tdt = t - surf->texturemins[T_AX];

        if ((tds > surf->extents[S_AX]) ||
            (tdt > surf->extents[T_AX])
            ) continue;

        if (surf->samples) {
            fixed8_t r = 0;
            uint8_p lightmap =
                surf->samples + (ptrdiff_t)(
                    FIXED4_TO_INT(tdt) * (FIXED4_TO_INT(surf->extents[S_AX]) + 1) +
                    FIXED4_TO_INT(tds));
            ptrdiff_t lmStep = (ptrdiff_t)(
                (FIXED4_TO_INT(surf->extents[S_AX]) + 1) *
                (FIXED4_TO_INT(surf->extents[T_AX]) + 1)
                );
            for (int maps = 0; (maps < MAXLIGHTMAPS) && (surf->styles[maps] != 0xFF); maps++) {
                r += (*lightmap) * d_lightstylevalue[surf->styles[maps]];
                lightmap += lmStep;
            }

            return FIXED8_TO_INT(r);
        }
        return CONTENTS_NODE;
    }

    return RecursiveLightPoint(node->children[!fSide], mid, end);    // go down back side
}



int R_LightPoint(vec3_t pnt) {
    if (!cl.worldmodel->lightdata)
        return 255;

    vec3_t end = pnt; { end.z -= 2048.0f; };
    int lp = RecursiveLightPoint(cl.worldmodel->nodes, pnt, end);

    if (lp == -1)
        lp = 0;

#ifndef GLQUAKE
    ClampLessThen(&lp, r_refdef.ambientLight);
#endif

    return lp;
}

