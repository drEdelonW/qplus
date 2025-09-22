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
// comndef.h  -- general definitions

#include <stdio.h>

#include "platformdefs.h"
#include "byte_t.h"
#include "qboolean.h"

#include "endian_tools.h"
#include "msg.h"
#include "q_tools.h"
#include "sizebuf.h"
#include "link.h"

//============================================================================
// #warning "COMMON.H" ADDED

// typedef char* cstring;
// typedef const char* cstring_ro;  // read-only
// typedef void* typeless_ptr;

#ifndef NULL
	#define NULL    ((typeless_ptr)0)
#endif

#if 0
    #define Q_MAXCHAR   ((char)0x7F)
    #define Q_MAXSHORT  ((int16_t)0x7FFF)
    #define Q_MAXINT    ((int)0x7FFFFFFF)
    #define Q_MAXLONG   ((int)0x7FFFFFFF)
    #define Q_MAXFLOAT  ((int)0x7FFFFFFF)

    #define Q_MINCHAR   ((char)0x80)
    #define Q_MINSHORT  ((int16_t)0x8000)
    #define Q_MININT    ((int)0x80000000)
    #define Q_MINLONG   ((int)0x80000000)
    #define Q_MINFLOAT  ((int)0x7FFFFFFF)
#else
    #include <stdint.h>
    #define Q_MAXCHAR   INT8_MAX      //  127
    #define Q_MAXSHORT  INT16_MAX     //  32767
    #define Q_MAXINT    INT32_MAX     //  2147483647
    #define Q_MAXLONG   INT32_MAX     //  совместимо с оригинальным Quake
    #define Q_MAXFLOAT  FLT_MAX       //  3.402823e+38F

    #define Q_MINCHAR   INT8_MIN      // -128
    #define Q_MINSHORT  INT16_MIN     // -32768
    #define Q_MININT    INT32_MIN     // -2147483648
    #define Q_MINLONG   INT32_MIN
    #define Q_MINFLOAT  (-FLT_MAX)
#endif


//============================================================================

extern	char		com_token[1024];
extern	qboolean	com_eof;

cstring COM_Parse(cstring data);

extern	int		com_argc;
extern	char	**com_argv;

int COM_CheckParm(cstring parm);
void COM_Init(cstring path);
void COM_InitArgv(int argc, cstring *argv);

cstring COM_SkipPath(cstring pathname);
void COM_StripExtension(cstring in, cstring out);
void COM_FileBase(cstring in, cstring out);
void COM_DefaultExtension(cstring path, cstring extension);

cstring va(cstring format, ...);
// does a varargs printf into a temp buffer


//============================================================================

extern int com_filesize;
struct cache_user_s;

extern	char	com_gamedir[MAX_OSPATH];

void COM_WriteFile(cstring filename, typeless_ptr data, int len);
int COM_OpenFile(cstring filename, int* hndl);
int COM_FOpenFile(cstring filename, FILE** file);
void COM_CloseFile(int h);

uint8_t* COM_LoadStackFile(cstring path, typeless_ptr buffer, int bufsize);
uint8_t* COM_LoadTempFile(cstring path);
uint8_t* COM_LoadHunkFile(cstring path);
void COM_LoadCacheFile(cstring path, struct cache_user_s *cu);


extern	struct cvar_s	registered;

extern qboolean		standard_quake, rogue, hipnotic;
