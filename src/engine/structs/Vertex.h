#pragma once

#include "vector.h"
//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
    vec3_t  position;
} mVertex_t;
typedef mVertex_t* mVertex_p;


typedef struct {
    vec3_t point;
} dVertex_t;
typedef dVertex_t* dVertex_p;
