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
// mathlib.c -- math primitives

#include "mathlib.h"
#include <math.h>
#include <string.h>
#ifndef PARANOID
#   include "host.h"    // void Host_SysError(cString error, ...);
#endif


vec3_t vec3_origin = { .x = 0.0f, .y = 0.0f, .z = 0.0f };  // TODO: move to more specific place
uint32_t nanmask = 0xFF << 23;

/*-----------------------------------------------------------------*/



/*-----------------------------------------------------------------*/




#ifdef _WIN32
#pragma optimize( "", off )
#endif


void RotatePointAroundVector(vec3_p dst, const vec3_t dir, const vec3_t point, float degrees) {
    vec3_t vf = dir; 
    // {
    //     .x = dir.x,
    //     .y = dir.y,
    //     .z = dir.z
    // };
    vec3_t vr = PerpendicularVector(dir);
    vec3_t vup = CrossProduct(vr, vf);

    mat3_t m = {
        .m = {
            {vr.x, vup.x, vf.x},
            {vr.y, vup.y, vf.y},
            {vr.z, vup.z, vf.z}
        }
    };

    mat3_t im;

    im.rows[0] = (vec3_t){{m.m[0][0], m.m[1][0], m.m[2][0]}};
    im.rows[1] = (vec3_t){{m.m[0][1], m.m[1][1], m.m[2][1]}};
    im.rows[2] = (vec3_t){{m.m[0][2], m.m[1][2], m.m[2][2]}};

    float angle_rad = DEG2RAD(degrees);
    float c = cos(angle_rad);
    float s = sin(angle_rad);

    mat3_t zrot = {
        .m = {
            {   c,    s, 0.0f},
            {  -s,    c, 0.0f},
            {0.0f, 0.0f, 1.0f}
        }
    };

    mat3_t tmpmat; R_ConcatRotations(&m, &zrot, &tmpmat);
    mat3_t rot;    R_ConcatRotations(&tmpmat, &im, &rot);

    for (int i = 0; i < VECT_DIM; i++) {
        dst->v[i] =
            rot.m[i][0] * point.x +
            rot.m[i][1] * point.y +
            rot.m[i][2] * point.z;
    }
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif


// double sqrt(double x);


int Q_log2(int val) {
    int answer = 0;
    while ((val = HALF(val)))
        answer++;
    return answer;
}


/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations(const mat3_p in1, const mat3_p in2, mat3_p out) {
#if 0
    out[0][0] =
        in1[0][0] * in2[0][0] +
        in1[0][1] * in2[1][0] +
        in1[0][2] * in2[2][0];
    out[0][1] =
        in1[0][0] * in2[0][1] +
        in1[0][1] * in2[1][1] +
        in1[0][2] * in2[2][1];
    out[0][2] =
        in1[0][0] * in2[0][2] +
        in1[0][1] * in2[1][2] +
        in1[0][2] * in2[2][2];

    out[1][0] =
        in1[1][0] * in2[0][0] +
        in1[1][1] * in2[1][0] +
        in1[1][2] * in2[2][0];
    out[1][1] =
        in1[1][0] * in2[0][1] +
        in1[1][1] * in2[1][1] +
        in1[1][2] * in2[2][1];
    out[1][2] =
        in1[1][0] * in2[0][2] +
        in1[1][1] * in2[1][2] +
        in1[1][2] * in2[2][2];

    out[2][0] =
        in1[2][0] * in2[0][0] +
        in1[2][1] * in2[1][0] +
        in1[2][2] * in2[2][0];
    out[2][1] =
        in1[2][0] * in2[0][1] +
        in1[2][1] * in2[1][1] +
        in1[2][2] * in2[2][1];
    out[2][2] =
        in1[2][0] * in2[0][2] +
        in1[2][1] * in2[1][2] +
        in1[2][2] * in2[2][2];

#else
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float sum = 0.0f;
            for (int k = 0; k < 3; k++)
                sum += in1->m[i][k] * in2->m[k][j];
            out->m[i][j] = sum;
        }
    }
#endif
}

/*
================
R_ConcatTransforms
================
*/
void R_ConcatTransforms(const mat3x4_p in1, const mat3x4_p in2, mat3x4_p out) {
#if 0
    out[0][0] =
        in1[0][0] * in2[0][0] +
        in1[0][1] * in2[1][0] +
        in1[0][2] * in2[2][0];
    out[0][1] =
        in1[0][0] * in2[0][1] +
        in1[0][1] * in2[1][1] +
        in1[0][2] * in2[2][1];
    out[0][2] =
        in1[0][0] * in2[0][2] +
        in1[0][1] * in2[1][2] +
        in1[0][2] * in2[2][2];
    out[0][3] =
        in1[0][0] * in2[0][3] +
        in1[0][1] * in2[1][3] +
        in1[0][2] * in2[2][3] +
        in1[0][3];

    out[1][0] =
        in1[1][0] * in2[0][0] +
        in1[1][1] * in2[1][0] +
        in1[1][2] * in2[2][0];
    out[1][1] =
        in1[1][0] * in2[0][1] +
        in1[1][1] * in2[1][1] +
        in1[1][2] * in2[2][1];
    out[1][2] =
        in1[1][0] * in2[0][2] +
        in1[1][1] * in2[1][2] +
        in1[1][2] * in2[2][2];
    out[1][3] =
        in1[1][0] * in2[0][3] +
        in1[1][1] * in2[1][3] +
        in1[1][2] * in2[2][3] +
        in1[1][3];

    out[2][0] =
        in1[2][0] * in2[0][0] +
        in1[2][1] * in2[1][0] +
        in1[2][2] * in2[2][0];
    out[2][1] =
        in1[2][0] * in2[0][1] +
        in1[2][1] * in2[1][1] +
        in1[2][2] * in2[2][1];
    out[2][2] =
        in1[2][0] * in2[0][2] +
        in1[2][1] * in2[1][2] +
        in1[2][2] * in2[2][2];
    out[2][3] =
        in1[2][0] * in2[0][3] +
        in1[2][1] * in2[1][3] +
        in1[2][2] * in2[2][3] +
        in1[2][3];
#else
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float sum = 0.0f;
            for (int k = 0; k < 3; k++)
                sum += in1->m[i][k] * in2->m[k][j];
            out->m[i][j] = sum;
        }
        float sum = in1->m[i][3];
        for (int k = 0; k < 3; k++)
            sum += in1->m[i][k] * in2->m[k][3];
        out->m[i][3] = sum;
    }
#endif
}

/*
===================
FloorDivMod

Returns mathematically correct (floor-based) quotient and remainder for
numer and denom, both of which should contain no fractional part. The
quotient must fit in 32 bits.
====================
*/

void FloorDivMod(double numer, double denom, int* quotient, int* rem) {
    int  q, r;

#ifndef PARANOID
    if (denom <= 0.0)       Host_SysError("FloorDivMod: bad denominator %d\n", denom);

#   if 0
    if ((floor(numer) != numer) ||
        (floor(denom) != denom)
        )        Host_SysError("FloorDivMod: non-integer numer or denom %f %f\n", numer, denom);
#   endif
#endif

    if (numer >= 0.0) {

        double x = floor(numer / denom);
        q = (int)x;
        r = (int)floor(numer - (x * denom));
    }
    else {
        //
        // perform operations with positive values, and fix mod to make floor-based
        //
        double x = floor(-numer / denom);
        q = -(int)x;
        r = (int)floor(-numer - (x * denom));
        if (r != 0) {
            q--;
            r = (int)denom - r;
        }
    }

    *quotient = q;
    *rem = r;
}


/*
===================
GreatestCommonDivisor
====================
*/
int GreatestCommonDivisor(int i1, int i2) {
    if (i1 > i2) {
        if (i2 == 0)
            return (i1);
        return GreatestCommonDivisor(i2, i1 % i2);
    }
    else {
        if (i1 == 0)
            return (i2);
        return GreatestCommonDivisor(i1, i2 % i1);
    }
}


