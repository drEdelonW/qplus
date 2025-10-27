#pragma once

#include "types.h"
#include "modelgen.h"


#define ALIAS_BASE_SIZE_RATIO   (1.0 / 11.0)
                    // normalizing factor so player model works out to about
                    //  1 pixel per triangle
/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct {
    AliasFrameType_t    type;
    TriVertx_t          bboxmin;
    TriVertx_t          bboxmax;
    int32_t             frame;
    char                name[16];
} mAliasFrameDesc_t;
typedef mAliasFrameDesc_t* mAliasFrameDesc_p;


typedef struct {
    AliasSkinType_t type;
    TypeLess_ptr    pcachespot;
    int32_t         skin;
} mAliasSkinDesc_t;
typedef mAliasSkinDesc_t* mAliasSkinDesc_p;

typedef struct {
    TriVertx_t  bboxmin;
    TriVertx_t  bboxmax;
    int32_t     frame;
} mAliasGroupFrameDesc_t;

typedef struct {
    int32_t                 numframes;
    int32_t                 intervals;
    mAliasGroupFrameDesc_t  frames[1];
} mAliasGroup_t;
typedef mAliasGroup_t* mAliasGroup_p;

typedef struct {
    int32_t numskins;
    int32_t intervals;
    mAliasSkinDesc_t    skindescs[1];
} mAliasSkinGroup_t;
typedef mAliasSkinGroup_t* mAliasSkinGroup_p;

typedef struct {
    int32_t model;
    int32_t stverts;
    int32_t skindesc;
    int32_t triangles;
    mAliasFrameDesc_t frames[1];
} AliasHdr_t;
typedef AliasHdr_t* AliasHdr_p;
