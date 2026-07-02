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
#include "view.h"
#include "client.h"
#include "mathlib.h"

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
extern int  r_dlightframecount;
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
        if ((l->die < GetClSimTime()) ||
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




