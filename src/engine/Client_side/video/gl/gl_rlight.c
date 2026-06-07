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

    vec3_t v; VectorSubtract(light->origin, r_origin, v);
    if (Length(v) < rad) {    // view is inside the dlight
        AddLightBlend(1, 0.5, 0, light->radius * 0.0003);
        return;
    }

    glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.2, 0.1, 0.0);
        for (int i = 0; i < 3; i++)
            v[i] = light->origin[i] - vpn[i] * rad;

        glVertex3fv(v);
        glColor3f(0, 0, 0);
        for (int i = 16; i >= 0; i--) {
            float a = i / 16.0 * M_PI * 2;
            for (int j = 0; j < 3; j++)
                v[j] = light->origin[j] +
                vright[j] * cos(a) * rad +
                vup[j] * sin(a) * rad;
            glVertex3fv(v);
        }
    } glEnd();
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights() {
    if (!gl_flashblend.value)
        return;

    r_dlightframecount = r_framecount + 1;    // because the count hasn't
    //  advanced yet for this frame
    glDepthMask(0);
    glDisable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    dLight_p l = cl_dlights;
    for (int i = 0; i < MAX_DLIGHTS; i++, l++) {
        if (l->die < cl.time || !l->radius)
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
    if (node->contents < 0)
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
    if (node->contents < 0)     return -1;        // didn't hit anything
    // calculate mid point

    // FIXME: optimize for axial
    mPlane_p plane = node->plane;
    float front = DotProduct(start, plane->normal) - plane->dist;
    float back = DotProduct(end, plane->normal) - plane->dist;
    int side = front < 0;

    if ((back < 0) == side)
        return RecursiveLightPoint(node->children[side], start, end);

    float frac = front / (front - back);
    vec3_t mid = {
        start[0] + (end[0] - start[0]) * frac,
        start[1] + (end[1] - start[1]) * frac,
        start[2] + (end[2] - start[2]) * frac
    };

    // go down front side
    int r = RecursiveLightPoint(node->children[side], start, mid);
    if (r >= 0)                 return r;        // hit something
    if ((back < 0) == side)     return -1;        // didn't hit anuthing

    // check for impact on this node
    VectorCopy(mid, lightspot);
    lightplane = plane;

    mSurface_p surf = cl.worldmodel->surfaces + node->firstsurface;
    for (int i = 0; i < node->numsurfaces; i++, surf++) {
        if (surf->flags & SURF_DRAWTILED)
            continue;    // no lightmaps

        mTexInfo_p tex = surf->texinfo;

        int s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
        int t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];;

        if ((s < surf->texturemins[0]) ||
            (t < surf->texturemins[1])
            ) continue;

        int ds = s - surf->texturemins[0];
        int dt = t - surf->texturemins[1];

        if ((ds > surf->extents[0]) ||
            (dt > surf->extents[1])
            ) continue;

        if (!surf->samples)     return 0;

        ds >>= 4;
        dt >>= 4;

        byte* lightmap = surf->samples;
        r = 0;
        if (lightmap) {

            lightmap += dt * ((surf->extents[0] >> 4) + 1) + ds;

            for (int maps = 0; (maps < MAXLIGHTMAPS) && (surf->styles[maps] != 255); maps++) {
                uint32_t scale = d_lightstylevalue[surf->styles[maps]];
                r += *lightmap * scale;
                lightmap += ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);
            }

            r >>= 8;
        }

        return r;
    }

    // go down back side
    return RecursiveLightPoint(node->children[!side], mid, end);
}

int R_LightPoint(vec3_t p) {
    if (!cl.worldmodel->lightdata)
        return 255;

    vec3_t end = {
        p[0],
        p[1],
        p[2] - 2048
    };

    int r = RecursiveLightPoint(cl.worldmodel->nodes, p, end);

    if (r == -1)
        r = 0;

    return r;
}

