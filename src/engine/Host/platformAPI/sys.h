#pragma once
/*
Copyright(C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys.h -- non-portable functions
#include "types.h"
#include "qTime.h"

#ifdef __cplusplus
extern "C" {
#endif

    //
    // file IO
    //

    // returns the file size
    // return -1 if file is not present
    // the file should be in BINARY mode for stupid OSs that care
    int Sys_FileOpenRead(cStringRO path, int* hndl);
    int Sys_FileOpenWrite(cStringRO path);
    void Sys_FileClose(int handle);

    void Sys_FileSeek(int handle, int position);
    int Sys_FileRead(int handle, TypeLess_ptr dest, size_t count);
    int Sys_FileWrite(int handle, TypeLess_ptr data, size_t count);
    int Sys_FileTime(cStringRO path);
    void Sys_mkdir(cStringRO path);

    //
    // memory protection
    //
    void Sys_MakeCodeWriteable(uintptr_t startaddr, size_t length);

    //
    // system IO
    //
    void Sys_DebugLog(cStringRO file, cStringRO fmt, ...);
    Q_NORETURN void Sys_Error(cStringRO error, ...);   // an error will cause the entire program to exit
    void Sys_Printf(cStringRO fmt, ...);    // send text to the console

    void Sys_Quit();
    LegacyTimeStamp_t Sys_FloatTime();
    cString Sys_ConsoleInput();

    void Sys_Sleep();   // called to yield for a little bit so as not to hog cpu when paused or debugging
    void Sys_SendKeyEvents();   // Perform Key_Event() callbacks until the input que is empty

    void Sys_LowFPPrecision();
    void Sys_HighFPPrecision();
    void Sys_SetFPCW();

#ifdef __cplusplus
}
#endif