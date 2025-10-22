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
// spritegn.h: header file for sprite generation program
//

// **********************************************************
// * This file must be identical in the spritegen directory *
// * and in the Quake directory, because it's used to       *
// * pass data from one to the other via .spr files.        *
// **********************************************************

//-------------------------------------------------------
// This program generates .spr sprite package files.
// The format of the files is as follows:
//
// dSprite_t file header structure
// <repeat dSprite_t.numframes times>
//   <if spritegroup, repeat dSpriteGroup_t.numframes times>
//     dSpriteFrame_t frame header structure
//     sprite bitmap
//   <else (single sprite frame)>
//     dSpriteFrame_t frame header structure
//     sprite bitmap
// <endrepeat>
//-------------------------------------------------------

#ifdef INCLUDELIBS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "dictlib.h"
#include "trilib.h"
#include "lbmlib.h"
#include "mathlib.h"

#endif

#define SPRITE_VERSION	1

// must match definition in modelgen.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum {
    ST_SYNC = 0,
    ST_RAND
} SyncType_t;
#endif

// TODO: shorten these?
typedef struct {
    int32_t     ident;
    int32_t     version;
    int32_t     type;
    float       boundingradius;
    int32_t     width;
    int32_t     height;
    int32_t     numframes;
    float       beamlength;
    SyncType_t  synctype;
} dSprite_t;
typedef dSprite_t* dSprite_p;

typedef enum {
    SPR_VP_PARALLEL_UPRIGHT   = 0, // viewplane parallel, upright
    SPR_FACING_UPRIGHT        = 1, // always faces viewer, upright
    SPR_VP_PARALLEL           = 2, // viewplane parallel, rotates with view
    SPR_ORIENTED              = 3, // fully oriented in 3D
    SPR_VP_PARALLEL_ORIENTED  = 4  // viewplane parallel, but oriented
} SpriteType_t;

typedef struct {
    int32_t origin[2];
    int32_t width;
    int32_t height;
} dSpriteFrame_t;
typedef dSpriteFrame_t* dSpriteFrame_p;

typedef struct {
    int32_t numframes;
} dSpriteGroup_t;
typedef dSpriteGroup_t* dSpriteGroup_p;

typedef struct {
    float   interval;
} dspriteinterval_t;
typedef dspriteinterval_t* dspriteinterval_p;

typedef enum {
    SPR_SINGLE = 0,
    SPR_GROUP
} SpriteFrameType_t;

typedef struct {
    SpriteFrameType_t	type;
} dSpriteFrameType_t;
typedef dSpriteFrameType_t* dSpriteFrameType_p;

#define IDSPRITEHEADER	(('P' << 24) + ('S' << 16) + ('D' << 8) + 'I')
// little-endian "IDSP"

