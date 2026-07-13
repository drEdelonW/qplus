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
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void PF_vectoyaw() {
    vec3_t value1 = G_VECTOR(OFS_PARM0);
    Angle_t yaw;
    if ((value1.y == 0.f) &&
        (value1.x == 0.f)
        )   yaw = 0.f;
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
    Basis_t bs = GetBasis(G_ANGLES(OFS_PARM0));
    pr_global_struct->v_forward = bs.forward;
    pr_global_struct->v_right = bs.right;
    pr_global_struct->v_up = bs.up;
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
void PF_rint() {
    float f = G_FLOAT(OFS_PARM0);
    G_FLOAT(OFS_RETURN) = (float)((int)(f + ((f > 0.f) ? 0.5f : -0.5f)));
}
void PF_normalize() { vec3_t value1 = G_VECTOR(OFS_PARM0); VectorNormalize(&value1); G_VECTOR(OFS_RETURN) = value1; }
void PF_vlen() { PR_Freturn Length(G_VECTOR(OFS_PARM0)); }
void PF_random() { G_FLOAT(OFS_RETURN) = (float)(rand() & 0x7fff) / ((float)0x7fff); }
void PF_floor() { G_FLOAT(OFS_RETURN) = floorf(G_FLOAT(OFS_PARM0)); }
void PF_ceil() { G_FLOAT(OFS_RETURN) = ceilf(G_FLOAT(OFS_PARM0)); }
void PF_fabs() { G_FLOAT(OFS_RETURN) = fabsf(G_FLOAT(OFS_PARM0)); }
void PF_sin() { G_FLOAT(OFS_RETURN) = sinf(G_FLOAT(OFS_PARM0)); }
void PF_cos() { G_FLOAT(OFS_RETURN) = cosf(G_FLOAT(OFS_PARM0)); }
void PF_sqrt() { G_FLOAT(OFS_RETURN) = sqrtf(G_FLOAT(OFS_PARM0)); }
