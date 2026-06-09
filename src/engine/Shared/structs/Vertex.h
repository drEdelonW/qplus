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
#if 0
    float   fv[3];  // viewspace x, y
#else
    vec3_t  fv;     // viewspace x, y
#endif
} AuxVert_t;
typedef AuxVert_t* AuxVert_p;

// flags in FinalVert_t.flags

typedef enum alias_clip_flags_e {
    ALIAS_LEFT_CLIP     = 0x0001u,
    ALIAS_TOP_CLIP      = 0x0002u,
    ALIAS_RIGHT_CLIP    = 0x0004u,
    ALIAS_BOTTOM_CLIP   = 0x0008u,
    ALIAS_XY_CLIP_MASK  = 0x000Fu, //  must be kept in sync
    ALIAS_Z_CLIP        = 0x0010u,

    // must stay in sync with d_ifacea.h and modelgen.h
    ALIAS_ONSEAM        = 0x0020u,  // also defined in modelgen.h
} AliasClipFlags_f;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct FinalVert_s {
    int     v[6];   // u, v, s, t, l, 1/z
#if 0
    int                 flags;  //alias_clip_flags_t
#else
    AliasClipFlags_f    flags;  //alias_clip_flags_t
#endif
    float   reserved;
} FinalVert_t;
typedef FinalVert_t* FinalVert_p;


typedef struct {
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
} stVert_t;
typedef stVert_t* stVert_p;