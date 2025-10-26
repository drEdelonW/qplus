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

void Sys_Error(cString error, ...);

/* extern */ vec3_t vec3_origin = { 0, 0, 0 };
/* extern */ int nanmask = 255 << 23;

/*-----------------------------------------------------------------*/

#define DEG2RAD(a) (a * M_PI) / 180.0F

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal) {
    float inv_denom = 1.0F / DotProduct(normal, normal);
    float d = DotProduct(normal, p) * inv_denom;

    vec3_t n;
    n[0] = normal[0] * inv_denom;
    n[1] = normal[1] * inv_denom;
    n[2] = normal[2] * inv_denom;

    dst[0] = p[0] - d * n[0];
    dst[1] = p[1] - d * n[1];
    dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t dst, const vec3_t src) {
    float minelem = 1.0F;

    /*
    ** find the smallest magnitude axially aligned vector
    */
    int pos = 0;
    for (int i = 0; i < VECT_DIM; i++) {
        if (fabs(src[i]) < minelem) {
            pos = i;
            minelem = fabs(src[i]);
        }
    }
    vec3_t tempvec = { 0.0F, 0.0F, 0.0F };
    tempvec[pos] = 1.0F;

    /*
    ** project the point onto the plane defined by src
    */
    ProjectPointOnPlane(dst, tempvec, src);

    /*
    ** normalize the result
    */
    VectorNormalize(dst);
}

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

    float zrot[3][3];
    float rot[3][3];
    memset(zrot, 0, sizeof(zrot));
    zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

    zrot[0][0] = cos(DEG2RAD(degrees));
    zrot[0][1] = sin(DEG2RAD(degrees));
    zrot[1][0] = -sin(DEG2RAD(degrees));
    zrot[1][1] = cos(DEG2RAD(degrees));

    float tmpmat[3][3];
    R_ConcatRotations(m, zrot, tmpmat);
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

/*-----------------------------------------------------------------*/


float anglemod(float a) {
#if 0
    if (a >= 0) a -= 360 * (int)(a / 360);
    else        a += 360 * (1 + (int)(-a / 360));
#endif
    a = (360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535);
    return a;
}





void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
    float  angle;

    angle = angles[YAW] * (M_PI * 2 / 360);
    float sy = sin(angle);
    float cy = cos(angle);
    angle = angles[PITCH] * (M_PI * 2 / 360);
    float sp = sin(angle);
    float cp = cos(angle);
    angle = angles[ROLL] * (M_PI * 2 / 360);
    float sr = sin(angle);
    float cr = cos(angle);

    forward[0] = cp * cy;
    forward[1] = cp * sy;
    forward[2] = -sp;
    right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
    right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
    right[2] = -1 * sr * cp;
    up[0] = (cr * sp * cy + -sr * -sy);
    up[1] = (cr * sp * sy + -sr * cy);
    up[2] = cr * cp;
}

bool VectorCompare(vec3_t v1, vec3_t v2) {
    for (int i = 0; i < VECT_DIM; i++)
        if (v1[i] != v2[i])
            return false;
    return true;
}

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc) {
    vecc[0] = veca[0] + scale * vecb[0];
    vecc[1] = veca[1] + scale * vecb[1];
    vecc[2] = veca[2] + scale * vecb[2];
}


vec_t _DotProduct(vec3_t v1, vec3_t v2) {
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out) {
    out[0] = veca[0] - vecb[0];
    out[1] = veca[1] - vecb[1];
    out[2] = veca[2] - vecb[2];
}

void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out) {
    out[0] = veca[0] + vecb[0];
    out[1] = veca[1] + vecb[1];
    out[2] = veca[2] + vecb[2];
}

void _VectorCopy(vec3_t in, vec3_t out) {
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross) {
    cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
    cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
    cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

double sqrt(double x);

vec_t Length(vec3_t v) {
    float length = 0;
    for (int i = 0; i < VECT_DIM; i++)
        length += v[i] * v[i];
    length = sqrt(length);  // FIXME

    return length;
}

float VectorNormalize(vec3_t v) {
    float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    length = sqrt(length);  // FIXME

    if (length) {
        float ilength = 1 / length;
        v[0] *= ilength;
        v[1] *= ilength;
        v[2] *= ilength;
    }

    return length;

}

void VectorInverse(vec3_t v) {
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
}

void VectorScale(vec3_t in, vec_t scale, vec3_t out) {
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
}


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


/*
================
R_ConcatTransforms
================
*/
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
    if (denom <= 0.0)
        Sys_Error("FloorDivMod: bad denominator %d\n", denom);

    // if ((floor(numer) != numer) || (floor(denom) != denom))
    //  Sys_Error ("FloorDivMod: non-integer numer or denom %f %f\n",
    //    numer, denom);
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


#if !id386

// TODO: move to nonintel.c

// /*
// ===================
// Invert24To16

// Inverts an 8.24 value to a 16.16 value
// ====================
// */

// fixed16_t Invert24To16(fixed16_t val) {
//     if (val < 256)
//         return (0xFFFFFFFF);

//     return (fixed16_t)
//         (((double)0x10000 * (double)0x1000000 / (double)val) + 0.5);
// }

#endif
