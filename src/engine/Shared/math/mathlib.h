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
#include "vector.h"
#include "angle.h"

// #error

#define	ON_EPSILON		0.1			// point on plane side epsilon

extern vec3_t vec3_origin;    // TODO: move to more specific place

extern uint32_t nanmask;
#define IS_NAN(x) (((*(uint32_t *)&x)&nanmask) == nanmask)

typedef union {
#if 0
    struct {
        vec3_t right;   // X axis basis vector
        vec3_t up;      // Y axis basis vector
        vec3_t forward; // Z axis basis vector
    };
#endif
    vec3_t rows[3];     // rows[0] = right, rows[1] = up, rows[2] = forward
    float m[3][3];
} mat3_t;
typedef mat3_t* mat3_p;

typedef union {
#if 0
    struct {
        vec3_t right;   // Right direction + X scaling
        vec3_t up;      // Up direction + Y scaling
        vec3_t forward; // Forward direction + Z scaling
        vec3_t origin;  // World position (translation)
    };
    vec3_t cols[4];     // Column access as vector array
#else
    // vec3_t rows[3];     // Каждая строка содержит 4 float (последний — координата)
#endif
    float m[3][4];      // Raw array access for R_ConcatTransforms
} mat3x4_t;
typedef mat3x4_t* mat3x4_p;


int     Q_log2(int val);

void    R_ConcatRotations(const mat3_p in1, const mat3_p in2, mat3_p out);
void    R_ConcatTransforms(const mat3x4_p in1, const mat3x4_p in2, mat3x4_p out);

vec3_t GetRotatePointAroundVector(const vec3_t dir, const vec3_t point, float degrees);

void    FloorDivMod(double numer, double denom, int* quotient, int* rem);
int     GreatestCommonDivisor(int i1, int i2);





