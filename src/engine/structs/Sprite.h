#pragma once

#include "vector.h"
#include "types.h"

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/
typedef struct {
    float u, v;
    float s, t;
    float zi;
} EmitPoint_t;
typedef EmitPoint_t* EmitPoint_p;


// FIXME: shorten these?
typedef struct mSpriteFrame_s {
    int32_t         width;
    int32_t         height;
    TypeLess_ptr    pcachespot;   // remove?
    float           up, down, left, right;
    uint8_t         pixels[4];
} mSpriteFrame_t;
typedef mSpriteFrame_t* mSpriteFrame_p;

typedef struct {
    int         nump;
    EmitPoint_p pverts; // there's room for an extra element at [nump],
    //  if the driver wants to duplicate element [0] at
    //  element [nump] to avoid dealing with wrapping
    mSpriteFrame_p  pspriteframe;
    vec3_t          vup, vright, vpn; // in worldspace
    float           nearzi;
} SpriteDesc_t;


typedef struct {
    int32_t         numframes;
    float_p         intervals;
    mSpriteFrame_p  frames[1];
} mSpriteGroup_t;
typedef mSpriteGroup_t* mSpriteGroup_p;


typedef enum {
    SPR_VP_PARALLEL_UPRIGHT   = 0, // viewplane parallel, upright
    SPR_FACING_UPRIGHT        = 1, // always faces viewer, upright
    SPR_VP_PARALLEL           = 2, // viewplane parallel, rotates with view
    SPR_ORIENTED              = 3, // fully oriented in 3D
    SPR_VP_PARALLEL_ORIENTED  = 4  // viewplane parallel, but oriented
} SpriteType_t;


typedef enum {
    SPR_SINGLE = 0,
    SPR_GROUP
} SpriteFrameType_t;


typedef struct {
    SpriteFrameType_t	type;
} dSpriteFrameType_t;
typedef dSpriteFrameType_t* dSpriteFrameType_p;

typedef struct {
    SpriteFrameType_t   type;
    mSpriteFrame_p      frameptr;
} mSpriteFrameDesc_t;
// typedef mSpriteFrame_t* mSpriteFrame_p;

typedef struct {
    SpriteType_t        type;
    int32_t             maxwidth;
    int32_t             maxheight;
    int32_t             numframes;
    float               beamlength;  // remove?
    TypeLess_ptr        cachespot;  // remove?
    mSpriteFrameDesc_t  frames[1];
} mSprite_t;
typedef mSprite_t* mSprite_p;
