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

#include "progs.h"
#include "progdefs.h"
#include "GlobVars.h"
#include "transform.h"
#include <stdlib.h>

/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void PF_normalize() {
    vec3_t value1 = G_VECTOR(OFS_PARM0);
    float new =
        value1.x * value1.x +
        value1.y * value1.y +
        value1.z * value1.z;
    new = (float)sqrt(new);

    vec3_t newvalue;
    if (new == 0.0f)   newvalue = (vec3_t){ .x = 0.0f, .y = 0.0f, .z = 0.0f };
    else {
        new = 1 / new;
        newvalue.x = value1.x * new;  // newvalue = value1 * new;
        newvalue.y = value1.y * new;
        newvalue.z = value1.z * new;
    }

    G_VECTOR(OFS_RETURN) = newvalue;
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
void PF_vlen() {
    vec3_t value1 = G_VECTOR(OFS_PARM0);

    float new =
        value1.x * value1.x +
        value1.y * value1.y +
        value1.z * value1.z;
    new = (float)sqrt(new);
#if 0
    G_FLOAT(OFS_RETURN) = new;
#else
    PR_Freturn new;
#endif
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void PF_vectoyaw() {
    vec3_t value1 = G_VECTOR(OFS_PARM0);

    Angle_t yaw;
    if ((value1.y == 0.f) &&
        (value1.x == 0.f))
        yaw = 0.f;
    else {
        yaw = (RAD2DEG(atan2f(value1.y, value1.x)));
        if (yaw < 0.f)    yaw += 360.f;
    }

    G_FLOAT(OFS_RETURN) = yaw;
}

/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
void PF_vectoangles() {
    vec3_t value1 = G_VECTOR(OFS_PARM0);
    ang3_t an = a3Zero;

    if ((value1.y == 0.f) &&
        (value1.x == 0.f)
        ) {
        an.yaw = 0.f;
        if (value1.z > 0.f)     an.pitch = 90.f;
        else                    an.pitch = 270.f;
    }
    else {
        an.yaw = (RAD2DEG(atan2f(value1.y, value1.x)));
        if (an.yaw < 0.f)    an.yaw += 360.f;

        float forward = sqrtf((value1.x * value1.x) + (value1.y * value1.y));
        an.pitch = (RAD2DEG(atan2f(value1.z, forward)));
        if (an.pitch < 0.f)  an.pitch += 360.f;
    }
    G_ANGLES(OFS_RETURN) = an;
}

/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void PF_makevectors() {
#if 0
    AngleVectors(
        G_VECTOR(OFS_PARM0),
        &pr_global_struct->v_forward,
        &pr_global_struct->v_right,
        &pr_global_struct->v_up
    );
#else
    Basis_t bs = GetBasis(G_ANGLES(OFS_PARM0));
    pr_global_struct->v_forward = bs.forward;
    pr_global_struct->v_right = bs.right;
    pr_global_struct->v_up = bs.up;

#endif
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
void PF_random() { G_FLOAT(OFS_RETURN) = (float)(rand() & 0x7fff) / ((float)0x7fff); }

void PF_rint() {
    float f = G_FLOAT(OFS_PARM0);
    if (f > 0)  G_FLOAT(OFS_RETURN) = (float)((int)(f + 0.5));
    else        G_FLOAT(OFS_RETURN) = (float)((int)(f - 0.5));
}
void PF_floor() { G_FLOAT(OFS_RETURN) = (float)floor(G_FLOAT(OFS_PARM0)); }
void PF_ceil() { G_FLOAT(OFS_RETURN) = (float)ceil(G_FLOAT(OFS_PARM0)); }

void PF_fabs() {
    float v = G_FLOAT(OFS_PARM0);
    G_FLOAT(OFS_RETURN) = (float)fabs(v);
}

void PF_sin() { G_FLOAT(OFS_RETURN) = sinf(G_FLOAT(OFS_PARM0)); }
void PF_cos() { G_FLOAT(OFS_RETURN) = cosf(G_FLOAT(OFS_PARM0)); }
void PF_sqrt() { G_FLOAT(OFS_RETURN) = sqrtf(G_FLOAT(OFS_PARM0)); }
