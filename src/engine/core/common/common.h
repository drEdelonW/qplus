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
#include "types.h"

#include "zone.h"

//============================================================================

extern bool		msg_suppress_1;		// suppresses resolution and cache size console output
//  an fullscreen DIB focus gain/loss
extern int32_t			current_skill;		// skill level for currently loaded level (in case
//  the user changes the cvar while the level is
//  running, this reflects the level actually in use)
// !!! if this is changed, it must be changed in quakedef.h too !!!
#define CACHE_SIZE	32		// used to align key data structures


//============================================================================

extern char com_token[1024];
extern bool com_eof;
#ifdef __cplusplus
extern "C" {
#endif
    cString COM_Parse(cString data);

    extern int com_argc;
    extern cStringArray com_argv;

    int COM_CheckParm(cStringRO parm);
    void COM_Init(cStringRO path);
    void COM_InitArgv(int argc, cStringArray argv);

    cString COM_SkipPath(cStringRO pathname);
    void COM_StripExtension(cStringRO in, cString out);
    void COM_FileBase(cStringRO in, cString out);
    void COM_DefaultExtension(cStringRO path, cString extension);

    cString va(cStringRO format, ...);
    // does a varargs printf into a temp buffer


    //============================================================================

    extern int32_t com_filesize;

    extern char com_gamedir[MAX_OSPATH];

    void COM_WriteFile(cStringRO filename, TypeLess_ptr data, int32_t len);
    int COM_OpenFile(cStringRO filename, int* hndl);
    int COM_FOpenFile(cStringRO filename, FILE** file);
    void COM_CloseFile(int h);

    uint8_p COM_LoadStackFile(cStringRO path, TypeLess_ptr buffer, int32_t bufsize);
    uint8_p COM_LoadTempFile(cStringRO path);
    uint8_p COM_LoadHunkFile(cStringRO path);
    void COM_LoadCacheFile(cStringRO path, CacheUser_p cu);

#ifdef __cplusplus
}
#endif
extern struct cvar_s registered;

extern bool  standard_quake, rogue, hipnotic;
