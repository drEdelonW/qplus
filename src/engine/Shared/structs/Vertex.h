#pragma once

#include "vector.h"
#include "fixed.h"
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
} dVertex_t;        STATIC_ASSERT_SIZE(dVertex_t, 3*4); // 12
typedef dVertex_t* dVertex_p;


typedef struct {
    vec3_t  fv;     // viewspace x, y
} AuxVert_t;
typedef AuxVert_t* AuxVert_p;

// flags in FinalVert_t.flags
typedef enum alias_clip_flags_e {
    ALIAS_NON_CLIP      = 0x0000u,
    ALIAS_LEFT_CLIP     = 0x0001u,
    ALIAS_TOP_CLIP      = 0x0002u,
    ALIAS_RIGHT_CLIP    = 0x0004u,
    ALIAS_BOTTOM_CLIP   = 0x0008u,
    ALIAS_XY_CLIP_MASK  = 0x000Fu, //  must be kept in sync
    ALIAS_Z_CLIP        = 0x0010u,

    // must stay in sync with d_ifacea.h and modelgen.h
    ALIAS_ONSEAM        = 0x0020u,  // also defined in modelgen.h
} AliasClipFlags_f;

typedef enum {
    FV_X = 0,   // screen x - plain int
    FV_Y,       // screen y - plain int
    FV_S,       // texture s - Q16.16
    FV_T,       // texture t - Q16.16
    FV_LIGHT,   // per-vertex light - Q16.16
    FV_ZI,      // 1/z - Q16.16
    FV_COUNT,
} FinalVertIdx_e;

typedef union VertAttr_u {
    struct {
        fixed16_t   x, y;      // screen pixel coords - plain int
        fixed16_t   s, t;      // texture coords - 16.16
        fixed16_t   light;     // per-vertex light - 16.16
        fixed16_t   zi;        // 1/z - 16.16
    };
    fixed16_t q16[FV_COUNT]; /* !!!MUST BE SIGNED!!! */  // u, v, s, t, l, 1/z  // (u, v), (s, t), light, iz
} VertAttr_t;
typedef VertAttr_t* VertAttr_p;


// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct FinalVert_s {
    VertAttr_t          vAttr;
    AliasClipFlags_f    flags;  //alias_clip_flags_t
#if 0
    float   reserved;   // no one use it 
#endif
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
    fixed16_t s;
    fixed16_t t;
} stVert_t;
typedef stVert_t* stVert_p;