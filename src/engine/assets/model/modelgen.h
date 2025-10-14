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
//
// modelgen.h: header file for model generation program
//

// *********************************************************
// * This file must be identical in the modelgen directory *
// * and in the Quake directory, because it's used to      *
// * pass data from one to the other via model files.      *
// *********************************************************

#ifdef INCLUDELIBS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "trilib.h"
#include "lbmlib.h"
#include "mathlib.h"

#endif

#include "types.h"
#include "mathlib.h"

#define ALIAS_VERSION 6

// enum { ALIAS_ONSEAM = 0x0020 };

// must match definition in spritegn.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum {
    ST_SYNC = 0,
    ST_RAND
} SyncType_t;
#endif


typedef struct {
    int32_t     ident;
    int32_t     version;
    vec3_t      scale;
    vec3_t      scale_origin;
    float       boundingradius;
    vec3_t      eyeposition;
    int32_t     numskins;
    int32_t     skinwidth;
    int32_t     skinheight;
    int32_t     numverts;
    int32_t     numtris;
    int32_t     numframes;
    SyncType_t  synctype;
    int32_t     flags;
    float       size;
} Mdl_t;
typedef Mdl_t* Mdl_p;

// TODO: could be shorts

typedef struct {
    int32_t onseam;
    int32_t s;
    int32_t t;
} stvert_t;
typedef stvert_t* stvert_p;


typedef struct dTriangle_s {
    int32_t facesfront;
    int32_t vertindex[3];
} dTriangle_t;
typedef dTriangle_t* dTriangle_p;

#define DT_FACES_FRONT  0x0010

// This mirrors trivert_t in trilib.h, is present so Quake knows how to
// load this data

typedef struct {
    uint8_t v[3];
    uint8_t lightnormalindex;
} TriVertx_t;
typedef TriVertx_t* TriVertx_p;


typedef struct {
    TriVertx_t  bboxmin;    // lightnormal isn't used
    TriVertx_t  bboxmax;    // lightnormal isn't used
    char        name[16];   // frame name from grabbing
} daliasframe_t;
typedef daliasframe_t* daliasframe_p;


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
} daliasinterval_t;
typedef daliasinterval_t* daliasinterval_p;


typedef struct {
    float interval;
} daliasskininterval_t;
typedef daliasskininterval_t* daliasskininterval_p;


typedef enum {
    ALIAS_SINGLE = 0,
    ALIAS_GROUP
} AliasFrameType_t;

typedef struct {
    AliasFrameType_t type;
} dAliasFrameType_t;
typedef dAliasFrameType_t* dAliasFrameType_p;


typedef enum {
    ALIAS_SKIN_SINGLE = 0,
    ALIAS_SKIN_GROUP
} AliasSkinType_t;

typedef struct {
    AliasSkinType_t type;
} dAliasSkinType_t;
typedef dAliasSkinType_t* dAliasSkinType_p;


#define IDPOLYHEADER    (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I')
// little-endian "IDPO"

