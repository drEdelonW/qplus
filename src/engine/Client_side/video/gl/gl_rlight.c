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
#include "qOpenGL.h"
#include "client.h"
#include "view.h"
#include "mathlib.h"
#include "model.h"

int r_dlightframecount;


/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight() {
    //
    // light animations
    // 'm' is normal light, 'a' is no light, 'z' is double bright
    int i = (int)(cl.time * 10);
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

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

void AddLightBlend(float r, float g, float b, float a2) {
    float a = v_blend[3] + a2 * (1 - v_blend[3]);

    a2 = a2 / a;

    v_blend[0] = v_blend[1] * (1 - a2) + r * a2;
    v_blend[1] = v_blend[1] * (1 - a2) + g * a2;
    v_blend[2] = v_blend[2] * (1 - a2) + b * a2;
    v_blend[3] = a;
}

void R_RenderDlight(dLight_p light) {
    float rad = light->radius * 0.35;

    if (Length(VectorSubtract(light->origin, r_origin)) < rad) {    // view is inside the dlight
        AddLightBlend(1.0f, 0.5f, 0.0f, light->radius * 0.0003f);
        return;
    }

    glBegin(GL_TRIANGLE_FAN); {

        glColor3f(0.2f, 0.1f, 0.0f);    glVertex3fv(VectorMA(light->origin, -rad, BS.forward).v);
        glColor3f(0.0f, 0.0f, 0.0f);
        for (int i = 16; i >= 0; i--) {
            float a = i / 16.0f * M_PI * 2.0f;

            glVertex3fv(VectorMA(VectorMA(light->origin,
                cos(a) * rad, BS.right),
                sin(a) * rad, BS.up
            ).v);
        }
    } glEnd();
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights() {
    if (!gl_flashblend.value)   return;

    r_dlightframecount = r_framecount + 1;    // because the count hasn't advanced yet for this frame
    glDepthMask(0);
    glDisable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    dLight_p l = cl_dlights;
    for (int i = 0; i < MAX_DLIGHTS; i++, l++) {
        if ((l->die < cl.time) ||
            !(l->radius)
            )
            continue;
        R_RenderDlight(l);
    }

    glColor3f(1, 1, 1);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(1);
}


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights(dLight_p light, int bit, mNode_p node) {
    if (node->contents < CONTENTS_NODE)
        return;

    mPlane_p splitplane = node->plane;
    float dist = DotProduct(light->origin, splitplane->normal) - splitplane->dist;

    if (dist > light->radius) {
        R_MarkLights(light, bit, node->children[0]);
        return;
    }
    if (dist < -light->radius) {
        R_MarkLights(light, bit, node->children[1]);
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

    R_MarkLights(light, bit, node->children[0]);
    R_MarkLights(light, bit, node->children[1]);
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights() {
    if (gl_flashblend.value)
        return;

    r_dlightframecount = r_framecount + 1;    // because the count hasn't
    //  advanced yet for this frame
    dLight_p l = cl_dlights;

    for (int i = 0; i < MAX_DLIGHTS; i++, l++) {
        if ((l->die < cl.time) ||
            (!l->radius)
            )
            continue;
        R_MarkLights(l, 1 << i, cl.worldmodel->nodes);
    }
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

mPlane_p    lightplane;
vec3_t      lightspot;

int RecursiveLightPoint(mNode_p node, vec3_t start, vec3_t end) {
    if (node->contents < CONTENTS_NODE)     return -1;        // didn't hit anything
    // calculate mid point

    // FIXME: optimize for axial
    mPlane_p plane = node->plane;
    float front = DotProduct(start, plane->normal) - plane->dist;
    float back = DotProduct(end, plane->normal) - plane->dist;
    int side = front < 0;

    if ((back < 0) == side)
        return RecursiveLightPoint(node->children[side], start, end);

    float frac = front / (front - back);

    // Linear interpolation: mid = start + (end - start) * frac
    vec3_t mid = VectorMA(start, frac, VectorSubtract(end, start));

    // go down front side
    int r = RecursiveLightPoint(node->children[side], start, mid);
    if (r >= 0)                 return r;        // hit something
    if ((back < 0) == side)     return -1;        // didn't hit anuthing

    // check for impact on this node
    lightspot = mid;
    lightplane = plane;

    mSurface_p surf = cl.worldmodel->surfaces + node->firstsurface;
    for (int i = 0; i < node->numsurfaces; i++, surf++) {
        if (surf->flags & SURF_DRAWTILED)
            continue;    // no lightmaps

        mTexInfo_p tex = surf->texinfo;

        int s = DotProduct(mid, tex->vecs[S_AX].vx) + tex->vecs[S_AX].offs;
        int t = DotProduct(mid, tex->vecs[T_AX].vx) + tex->vecs[T_AX].offs;

        if ((s < surf->texturemins[S_AX]) ||
            (t < surf->texturemins[T_AX])
            ) continue;

        fixed16_t ds = s - surf->texturemins[S_AX];
        fixed16_t dt = t - surf->texturemins[T_AX];

        if ((ds > surf->extents[S_AX]) ||
            (dt > surf->extents[T_AX])
            ) continue;

        if (!surf->samples)     return 0;

        ds = DIV16(ds);
        dt = DIV16(dt);

        byte* lightmap = surf->samples;
        r = 0;
        if (lightmap) {

            lightmap += dt * (DIV16(surf->extents[S_AX]) + 1) + ds;

            for (int maps = 0; (maps < MAXLIGHTMAPS) && (surf->styles[maps] != 0xFF); maps++) {
                uint32_t scale = d_lightstylevalue[surf->styles[maps]];
                r += *lightmap * scale;
                lightmap +=
                    (DIV16(surf->extents[S_AX]) + 1) *
                    (DIV16(surf->extents[T_AX]) + 1);
            }

            r = r >> 8;
        }

        return r;
    }

    // go down back side
    return RecursiveLightPoint(node->children[!side], mid, end);
}

int R_LightPoint(vec3_t p) {
    if (!cl.worldmodel->lightdata)
        return 255;

    vec3_t end = p;
    end.z -= 2048.0f;

    int r = RecursiveLightPoint(cl.worldmodel->nodes, p, end);

    if (r == -1)
        r = 0;

    return r;
}

