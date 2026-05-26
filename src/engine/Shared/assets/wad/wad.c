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
// wad.c

#include "wad.h"
#include <string.h>
#include "common.h"
#include "host.h"
#include "endian_tools.h"
#include "qPic.h"

#define LUMP_NAME_LEN   (16)

//===============
//   TYPES
//===============
typedef enum {
    CMP_NONE = 0u,
    CMP_LZSS = 1u
} CmpType;

typedef enum {
    TYP_NONE    = 0u,
    TYP_LABEL   = 1u,

    TYP_LUMPY   = 64u,   // base offset for grab command number
    TYP_PALETTE = 64u,
    TYP_QTEX    = 65u,
    TYP_QPIC    = 66u,
    TYP_SOUND   = 67u,
    TYP_MIPTEX  = 68u
} TypType;

typedef struct {
    int32_t filepos;
    int32_t disksize;
    int32_t size;           // uncompressed
    char    type;           // TypType ?
    char    compression;    // CmpType ?
    char    pad1;
    char    pad2;
    char    name[LUMP_NAME_LEN]; // must be null terminated
} LumpInfo_t;
typedef LumpInfo_t* LumpInfo_p;

static LumpInfo_p  _LumpsBase;
static int32_t     _NumLumps;
static uint8_p     _wadBase;

typedef struct {
    char    ID[4];		// should be WAD2 or 2DAW
    int32_t numLumps;
    int32_t infoTableOfs;
} WadInfo_t;
typedef WadInfo_t* WadInfo_p;


/*
    ==================
    W_CleanupName

    Lowercases name and pads with spaces and a terminating 0 to the length of
    LumpInfo_t->name.
    Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
    Space padding is so names can be printed nicely in tables.
    Can safely be performed in place.
    ==================
*/
void W_CleanupName(cStringRO in, cString out) {
    int i = 0;
    for (; i < LUMP_NAME_LEN; i++) {
        char c = in[i];
        if (!c) break;

        if ((c >= 'A') && (c <= 'Z'))   c += ('a' - 'A');
        out[i] = c;
    }

    for (; i < LUMP_NAME_LEN; i++)
        out[i] = 0x00;
}

/*
    ====================
    W_LoadWadFile
    ====================
*/
void W_LoadWadFile(cStringRO filename) {
    _wadBase = COM_LoadHunkFile(filename);
    if (!_wadBase)
        Host_SysError("W_LoadWadFile: couldn't load %s", filename);

    WadInfo_p header = (WadInfo_p)_wadBase;

    if (
        (header->ID[0] != 'W') ||
        (header->ID[1] != 'A') ||
        (header->ID[2] != 'D') ||
        (header->ID[3] != '2')
        )
        Host_SysError("Wad file %s doesn't have WAD2 id\n", filename);

    _NumLumps = LittleLong(header->numLumps);
    int infoTableOfs = LittleLong(header->infoTableOfs);
    _LumpsBase = (LumpInfo_p)(_wadBase + infoTableOfs);

    LumpInfo_p lump_p = _LumpsBase;
    for (uint32_t i = 0; i < _NumLumps; i++, lump_p++) {
        lump_p->filepos = LittleLong(lump_p->filepos);
        lump_p->size = LittleLong(lump_p->size);
        W_CleanupName(lump_p->name, lump_p->name);
        if (lump_p->type == TYP_QPIC)
            SwapPic((qPic_p)(_wadBase + lump_p->filepos));
    }
}


/*
    =============
    W_GetLumpinfo
    =============
*/
LumpInfo_p W_GetLumpinfo(cStringRO name) {
    char clean[LUMP_NAME_LEN];
    W_CleanupName(name, clean);

    LumpInfo_p lump_p = _LumpsBase;
    for (int32_t i = 0; i < _NumLumps; i++, lump_p++) {
        if (!strncmp(clean, lump_p->name, LUMP_NAME_LEN))
            return lump_p;
    }

    Host_SysError("W_GetLumpinfo: [%s] not found", name);
    return NULL;
}

TypeLess_ptr W_GetLumpName(cStringRO name) {
    return (TypeLess_ptr)(_wadBase + W_GetLumpinfo(name)->filepos);
}

TypeLess_ptr W_GetLumpNum(int32_t num) {
    if ((num < 0) ||
        (num > _NumLumps)
        )
        Host_SysError("W_GetLumpNum: bad number: %i", num);

    return (TypeLess_ptr)(_wadBase + (_LumpsBase + num)->filepos);
}

/*
    =============================================================================

    automatic byte swapping

    =============================================================================
*/


