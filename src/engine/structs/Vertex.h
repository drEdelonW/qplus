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

typedef struct {
    // float   fv[3];  // viewspace x, y
    vec3_t fv;  // viewspace x, y
} AuxVert_t;
typedef AuxVert_t* AuxVert_p;

// flags in FinalVert_t.flags

typedef enum alias_clip_flags_e {
    ALIAS_LEFT_CLIP     = 0x0001,
    ALIAS_TOP_CLIP      = 0x0002,
    ALIAS_RIGHT_CLIP    = 0x0004,
    ALIAS_BOTTOM_CLIP   = 0x0008,
    ALIAS_XY_CLIP_MASK  = 0x000F, //  must be kept in sync
    ALIAS_Z_CLIP        = 0x0010,

    // must stay in sync with d_ifacea.h and modelgen.h
    ALIAS_ONSEAM        = 0x0020,  // also defined in modelgen.h
} AliasClipFlags_f;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct FinalVert_s {
    int     v[6];   // u, v, s, t, l, 1/z
    // int     flags;  //alias_clip_flags_t
    AliasClipFlags_f    flags;  //alias_clip_flags_t
    float   reserved;
} FinalVert_t;
typedef FinalVert_t* FinalVert_p;


typedef struct PolyVert_s {
    float u, v;
    float zi;
    float s, t;
} PolyVert_t;
typedef PolyVert_t* PolyVert_p;

// enum { ALIAS_ONSEAM = 0x0020 };
// TODO: could be shorts

typedef struct {
    int32_t onseam;
    int32_t s;
    int32_t t;
} stvert_t;
typedef stvert_t* stvert_p;