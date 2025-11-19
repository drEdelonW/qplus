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
#include "host.h"    // void Host_SysError(cString error, ...);

vec3_t vec3_origin = { 0.0f, 0.0f, 0.0f };
uint32_t nanmask = 0xFF << 23;

/*-----------------------------------------------------------------*/



/*-----------------------------------------------------------------*/




#ifdef _WIN32
#pragma optimize( "", off )
#endif


void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees) {
    vec3_t vf = {
        dir[0],
        dir[1],
        dir[2]
    };
    vec3_t vr;  PerpendicularVector(vr, dir);
    vec3_t vup; CrossProduct(vr, vf, vup);
    float m[3][3];
    m[0][0] = vr[0];
    m[1][0] = vr[1];
    m[2][0] = vr[2];

    m[0][1] = vup[0];
    m[1][1] = vup[1];
    m[2][1] = vup[2];

    m[0][2] = vf[0];
    m[1][2] = vf[1];
    m[2][2] = vf[2];

    float im[3][3];    memcpy(im, m, sizeof(im));

    im[0][1] = m[1][0];
    im[0][2] = m[2][0];

    im[1][0] = m[0][1];
    im[1][2] = m[2][1];

    im[2][0] = m[0][2];
    im[2][1] = m[1][2];

    float zrot[3][3];   memset(zrot, 0, sizeof(zrot));
    float rot[3][3];
    zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

    zrot[0][0] = cos(DEG2RAD(degrees));
    zrot[0][1] = sin(DEG2RAD(degrees));
    zrot[1][0] = -sin(DEG2RAD(degrees));
    zrot[1][1] = cos(DEG2RAD(degrees));

    float tmpmat[3][3]; R_ConcatRotations(m, zrot, tmpmat);
    R_ConcatRotations(tmpmat, im, rot);

    for (int i = 0; i < VECT_DIM; i++) {
        dst[i] =
            rot[i][0] * point[0] +
            rot[i][1] * point[1] +
            rot[i][2] * point[2];
    }
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif


// double sqrt(double x);


int Q_log2(int val) {
    int answer = 0;
    while (val >>= 1)
        answer++;
    return answer;
}


/*
================
R_ConcatRotations
================
*/
#if 0
void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]) {
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

}
#else
void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float sum = 0.0f;
            for (int k = 0; k < 3; k++)
                sum += in1[i][k] * in2[k][j];
            out[i][j] = sum;
        }
    }
}
#endif

/*
================
R_ConcatTransforms
================
*/
#if 0
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]) {
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
}
#else
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float sum = 0.0f;
            for (int k = 0; k < 3; k++)
                sum += in1[i][k] * in2[k][j];
            out[i][j] = sum;
        }
        float sum = in1[i][3];
        for (int k = 0; k < 3; k++)
            sum += in1[i][k] * in2[k][3];
        out[i][3] = sum;
    }
}
#endif

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

    // if ((floor(numer) != numer) || (floor(denom) != denom))
    //  Host_SysError ("FloorDivMod: non-integer numer or denom %f %f\n", numer, denom);
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


