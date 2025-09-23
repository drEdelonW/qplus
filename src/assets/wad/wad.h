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
// wad.h
#include "zone.h"
#include "byte_t.h"

//===============
//   TYPES
//===============

#if 0
    #define	CMP_NONE		0
    #define	CMP_LZSS		1

    #define	TYP_NONE		0
    #define	TYP_LABEL		1

    #define	TYP_LUMPY		64				// 64 + grab command number
    #define	TYP_PALETTE		64
    #define	TYP_QTEX		65
    #define	TYP_QPIC		66
    #define	TYP_SOUND		67
    #define	TYP_MIPTEX		68
#else
typedef enum {
    CMP_NONE    = 0,
    CMP_LZSS    = 1
} cmp_type_t;

typedef enum {
    TYP_NONE    = 0,
    TYP_LABEL   = 1,

    TYP_LUMPY   = 64,   // base offset for grab command number
    TYP_PALETTE = 64,
    TYP_QTEX    = 65,
    TYP_QPIC    = 66,
    TYP_SOUND   = 67,
    TYP_MIPTEX  = 68
} typ_type_t;
#endif

typedef struct {
    int     width;
    int     height;
    byte    data[4];			// variably sized
} qpic_t;
typedef qpic_t* qpic_p;



typedef struct {
	char    identification[4];		// should be WAD2 or 2DAW
	int     numlumps;
	int     infotableofs;
} wadinfo_t;
typedef wadinfo_t* wadinfo_p;

typedef struct {
	int			filepos;
	int			disksize;
	int			size;					// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[16];				// must be null terminated
} lumpinfo_t;
typedef lumpinfo_t* lumpinfo_p;

extern	int			wad_numlumps;
extern	lumpinfo_p	wad_lumps;
extern	byte*		wad_base;

void	W_LoadWadFile(cstring filename);
void    W_CleanupName(cstring in, cstring out);
lumpinfo_p     W_GetLumpinfo(cstring name);
typeless_ptr    W_GetLumpName(cstring name);
typeless_ptr    W_GetLumpNum(int num);

void SwapPic(qpic_p pic);
