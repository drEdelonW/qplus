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

#ifdef GLTEST

typedef struct {
    Plane_p plane;
    vec3_t  origin;
    vec3_t  normal;
    vec3_t  up;
    vec3_t  right;
    vec3_t  reflect;
    float   length;
} puff_t;

#define MAX_PUFFS   64

puff_t  puffs[MAX_PUFFS];


void Test_Init() {}



static Plane_t _junk;
Plane_p HitPlane(vec3_t start, vec3_t end) {
    trace_t trace = {   // fill in a default trace
        .fraction = 1.f,
        .allsolid = true,
        .endpos = end,
    };

    SV_RecursiveHullCheck(cl.worldmodel->hulls, 0, 0.f, 1.f, start, end, &trace);

    _junk = trace.plane;
    return &_junk;
}

void Test_Spawn(vec3_t origin) {
    puff_p p = puffs;
    int i = 0;
    for (; i < MAX_PUFFS; i++, p++) {
        if (p->length <= 0)
            break;
    }
    if (i == MAX_PUFFS)
        return;

    vec3_t incoming = VectorSubtract(r_refdef.vieworg, origin);
    vec3_t temp = VectorSubtract(origin, incoming);
    Plane_p plane = HitPlane(r_refdef.vieworg, temp);

    VectorNormalize(incoming);
    float d = DotProduct(incoming, plane->normal);
    p->reflect = VectorSubtract(v3Zero, incoming);
    p->reflect = VectorMA(p->reflect, d * 2.0f, plane->normal);

    p->origin = origin;
    p->normal = plane->normal;

    p->up = CrossProduct(incoming, p->normal);
    p->right = CrossProduct(p->up, p->normal);

    p->length = 8;
}

void DrawPuff(puff_p p) {
    vec3_t pts[2][3];

    for (int i = 0; i < 2; i++) {
        float s, d;
        if (i == 1) {
            s = 6;
            d = p->length;
        }
        else {
            s = 2;
            d = 0;
        }

        pts[i][0] = VectorMA(VectorMA(p->origin, s, p->up), d, p->reflect);
        pts[i][1] = VectorMA(VectorMA(p->origin, s, p->right), d, p->reflect);
        pts[i][2] = VectorMA(VectorMA(p->origin, -s, p->right), d, p->reflect);
    }

    glColor3f(1, 0, 0);

#if 0
    glBegin(GL_LINES); {
        glVertex3fv(p->origin);
        glVertex3f(
            p->origin[0] + p->length * p->reflect[0],
            p->origin[1] + p->length * p->reflect[1],
            p->origin[2] + p->length * p->reflect[2]
        );

        glVertex3fv(pts[0][0]);
        glVertex3fv(pts[1][0]);

        glVertex3fv(pts[0][1]);
        glVertex3fv(pts[1][1]);

        glVertex3fv(pts[0][2]);
        glVertex3fv(pts[1][2]);

    } glEnd();
#endif

    glBegin(GL_QUADS); {
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            glVertex3fv(pts[0][j]);
            glVertex3fv(pts[1][j]);
            glVertex3fv(pts[1][i]);
            glVertex3fv(pts[0][i]);
        }
    } glEnd();

    glBegin(GL_TRIANGLES); {
        glVertex3fv(pts[1][0]);
        glVertex3fv(pts[1][1]);
        glVertex3fv(pts[1][2]);
    } glEnd();

    p->length -= host_frametime * 2.0f;
}


void Test_Draw() {
    puff_p p = puffs;
    for (int i = 0; i < MAX_PUFFS; i++, p++) {
        if (p->length > 0)
            DrawPuff(p);
    }
}

#endif
