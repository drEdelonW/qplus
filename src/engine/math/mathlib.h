#pragma once
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
// mathlib.h
#include <math.h>
#include "types.h"
#include "angles_indices.h"

#define	ON_EPSILON		0.1			// point on plane side epsilon

typedef float vec_t;
typedef enum axis_e {
    X_AX = 0,
    Y_AX = 1,
    Z_AX = 2,
    VECT_DIM = 3,
} axis_e;

typedef vec_t vec3_t[VECT_DIM];
typedef vec3_t* vec3_p;

typedef vec_t vec5_t[5];
typedef vec5_t* vec5_p;

#ifndef M_PI
#define M_PI  (3.14159265358979323846) /* matches value in gcc v2 math.h */
#endif

extern vec3_t vec3_origin;
extern uint32_t nanmask;

#define IS_NAN(x) (((*(uint32_t *)&x)&nanmask) == nanmask)

vec_t  _DotProduct(vec3_t v1, vec3_t v2);
#define DotProduct(x, y)    \
    (                       \
        x[0] * y[0] +       \
        x[1] * y[1] +       \
        x[2] * y[2]         \
    )

void   _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);
#define VectorSubtract(a, b, c) \
    {                           \
        c[0] = a[0] - b[0];     \
        c[1] = a[1] - b[1];     \
        c[2] = a[2] - b[2];     \
    }

void   _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);
#define VectorAdd(a, b, c)  \
    {                       \
        c[0] = a[0] + b[0]; \
        c[1] = a[1] + b[1]; \
        c[2] = a[2] + b[2]; \
    }

void   _VectorCopy(vec3_t in, vec3_t out);
#define VectorCopy(a, b)    \
    {                       \
        b[0] = a[0];        \
        b[1] = a[1];        \
        b[2] = a[2];        \
    }

void    VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

bool    VectorCompare(vec3_t v1, vec3_t v2);
vec_t   Length(vec3_t v);

void    CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross);

float   VectorNormalize(vec3_t v);  // returns vector length
void    VectorInverse(vec3_t v);
void    VectorScale(vec3_t in, vec_t scale, vec3_t out);
int     Q_log2(int val);

void    R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
void    R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void    FloorDivMod(double numer, double denom, int* quotient, int* rem);
int     GreatestCommonDivisor(int i1, int i2);

void    AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);

float   anglemod(float a);



