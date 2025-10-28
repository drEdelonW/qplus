#pragma once

#include "types.h"
#include "Triangle.h"

#define ALIAS_BASE_SIZE_RATIO   (1.0 / 11.0)
                    // normalizing factor so player model works out to about
                    //  1 pixel per triangle


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/
typedef enum {
    ALIAS_SINGLE = 0,
    ALIAS_GROUP
} AliasFrameType_t;

typedef struct {
    AliasFrameType_t type;
} dAliasFrameType_t;
typedef dAliasFrameType_t* dAliasFrameType_p;

typedef struct {
    AliasFrameType_t    type;
    TriVertx_t          bboxmin;
    TriVertx_t          bboxmax;
    int32_t             frame;
    char                name[16];
} mAliasFrameDesc_t;
typedef mAliasFrameDesc_t* mAliasFrameDesc_p;



typedef struct {
    int32_t model;
    int32_t stverts;
    int32_t skindesc;
    int32_t triangles;
    mAliasFrameDesc_t frames[1];
} AliasHdr_t;
typedef AliasHdr_t* AliasHdr_p;



typedef enum {
    ALIAS_SKIN_SINGLE = 0,
    ALIAS_SKIN_GROUP
} AliasSkinType_t;

typedef struct {
    AliasSkinType_t type;
} dAliasSkinType_t;
typedef dAliasSkinType_t* dAliasSkinType_p;

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
    TriVertx_t  bboxmin;    // lightnormal isn't used
    TriVertx_t  bboxmax;    // lightnormal isn't used
    char        name[16];   // frame name from grabbing
} dAliasFrame_t;
typedef dAliasFrame_t* dAliasFrame_p;


typedef struct {
    int32_t     numframes;
    TriVertx_t  bboxmin;    // lightnormal isn't used
    TriVertx_t  bboxmax;    // lightnormal isn't used
} dAliasGroup_t;
typedef dAliasGroup_t* dAliasGroup_p;


typedef struct {
    int32_t numskins;
} dAliasSkinGroup_t;
typedef dAliasSkinGroup_t* dAliasSkinGroup_p;


typedef struct {
    float interval;
} dAliasInterval_t;
typedef dAliasInterval_t* dAliasInterval_p;


typedef struct {
    float interval;
} dAliasSkinInterval_t;
typedef dAliasSkinInterval_t* dAliasSkinInterval_p;



