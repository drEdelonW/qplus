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


void Test_Init(void) {}



Plane_t junk;
Plane_p HitPlane(vec3_t start, vec3_t end) {
    trace_t trace;

    // fill in a default trace
    memset(&trace, 0, sizeof(trace_t));
    trace.fraction = 1;
    trace.allsolid = true;
    VectorCopy(end, trace.endpos);

    SV_RecursiveHullCheck(cl.worldmodel->hulls, 0, 0, 1, start, end, &trace);

    junk = trace.plane;
    return &junk;
}

void Test_Spawn(vec3_t origin) {
    // vec3_t  normal;

    puff_p p = puffs;
    int i = 0;
    for (; i < MAX_PUFFS; i++, p++) {
        if (p->length <= 0)
            break;
    }
    if (i == MAX_PUFFS)
        return;

    vec3_t incoming; VectorSubtract(r_refdef.vieworg, origin, incoming);
    vec3_t temp; VectorSubtract(origin, incoming, temp);
    Plane_p plane = HitPlane(r_refdef.vieworg, temp);

    VectorNormalize(incoming);
    float d = DotProduct(incoming, plane->normal);
    VectorSubtract(vec3_origin, incoming, p->reflect);
    VectorMA(p->reflect, d * 2, plane->normal, p->reflect);

    VectorCopy(origin, p->origin);
    VectorCopy(plane->normal, p->normal);

    CrossProduct(incoming, p->normal, p->up);

    CrossProduct(p->up, p->normal, p->right);

    p->length = 8;
}

void DrawPuff(puff_p p) {
    vec3_t pts[2][3];
    float  s, d;

    for (int i = 0; i < 2; i++) {
        if (i == 1) {
            s = 6;
            d = p->length;
        }
        else {
            s = 2;
            d = 0;
        }

        for (int j = 0; j < 3; j++) {
            pts[i][0][j] = p->origin[j] + p->up[j] * s + p->reflect[j] * d;
            pts[i][1][j] = p->origin[j] + p->right[j] * s + p->reflect[j] * d;
            pts[i][2][j] = p->origin[j] + -p->right[j] * s + p->reflect[j] * d;
        }
    }

    glColor3f(1, 0, 0);

#if 0
    glBegin(GL_LINES);
    glVertex3fv(p->origin);
    glVertex3f(p->origin[0] + p->length * p->reflect[0],
        p->origin[1] + p->length * p->reflect[1],
        p->origin[2] + p->length * p->reflect[2]);

    glVertex3fv(pts[0][0]);
    glVertex3fv(pts[1][0]);

    glVertex3fv(pts[0][1]);
    glVertex3fv(pts[1][1]);

    glVertex3fv(pts[0][2]);
    glVertex3fv(pts[1][2]);

    glEnd();
#endif

    glBegin(GL_QUADS);
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        glVertex3fv(pts[0][j]);
        glVertex3fv(pts[1][j]);
        glVertex3fv(pts[1][i]);
        glVertex3fv(pts[0][i]);
    }
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex3fv(pts[1][0]);
    glVertex3fv(pts[1][1]);
    glVertex3fv(pts[1][2]);
    glEnd();

    p->length -= host_frametime * 2;
}


void Test_Draw(void) {
    puff_p p = puffs;
    for (int i = 0; i < MAX_PUFFS; i++, p++) {
        if (p->length > 0)
            DrawPuff(p);
    }
}

#endif
