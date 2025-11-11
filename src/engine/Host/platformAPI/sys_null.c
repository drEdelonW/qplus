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
// sys_null.h -- null system driver to aid porting efforts

#include "sys.h"
// #include "errno.h"
#include "qparams.h"
#include "host.h"
#include "common.h"
#include "console.h"
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <linux/soundcard.h>
#include <stdio.h>
/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES             10
static FILE* _sys_handles[MAX_HANDLES];

int findhandle() {
    for (int i = 1; i < MAX_HANDLES; i++)
        if (!_sys_handles[i])
            return i;
    Sys_Error("out of handles");
    return -1;
}

/*
================
filelength
================
*/
int filelength(FILE* f) {
    int pos = ftell(f);
    fseek(f, 0, SEEK_END);
    int end = ftell(f);
    fseek(f, pos, SEEK_SET);

    return end;
}

__attribute__((weak)) int Sys_FileOpenRead(cStringRO path, int* hndl) {
    int i = findhandle();

    FILE* f = fopen(path, "rb");
    if (!f) {
        *hndl = -1;
        return -1;
    }
    _sys_handles[i] = f;
    *hndl = i;

    return filelength(f);
}

__attribute__((weak)) int Sys_FileOpenWrite(cStringRO path) {
    int i = findhandle();

    FILE* f = fopen(path, "wb");
    if (!f)
        Sys_Error("Error opening %s: %s", path, strerror(errno));
    _sys_handles[i] = f;

    return i;
}

__attribute__((weak)) void Sys_FileClose(int handle) {
    fclose(_sys_handles[handle]);
    _sys_handles[handle] = NULL;
}

__attribute__((weak)) void Sys_FileSeek(int handle, int position) { fseek(_sys_handles[handle], position, SEEK_SET); }

__attribute__((weak)) int Sys_FileRead(int handle, TypeLess_ptr dest, size_t count) { return fread(dest, 1, count, _sys_handles[handle]); }

__attribute__((weak)) int Sys_FileWrite(int handle, TypeLess_ptr data, size_t count) { return fwrite(data, 1, count, _sys_handles[handle]); }

__attribute__((weak)) int Sys_FileTime(cStringRO path) {
    FILE* f;

    f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return 1;
    }

    return -1;
}

__attribute__((weak)) void Sys_mkdir(cStringRO path) {
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

__attribute__((weak)) void Sys_MakeCodeWriteable(uintptr_t startaddr, size_t length) {}


__attribute__((weak)) void Sys_Error(cStringRO error, ...) {
    printf("Sys_Error: ");
    va_list argptr;    va_start(argptr, error);
    vprintf(error, argptr);
    va_end(argptr);
    printf("\n");

    exit(1);
}

__attribute__((weak)) void Sys_Printf(cStringRO fmt, ...) {
    va_list argptr;    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    va_end(argptr);
}

__attribute__((weak)) void Sys_Quit() { exit(0); }

__attribute__((weak)) double Sys_FloatTime() {
    static double _time;
    _time += 0.1;
    return _time;
}

__attribute__((weak)) cString Sys_ConsoleInput() { return NULL; }

__attribute__((weak)) void Sys_Sleep() {}
__attribute__((weak)) void Sys_SendKeyEvents() {}
__attribute__((weak)) void Sys_HighFPPrecision() {}
__attribute__((weak)) void Sys_LowFPPrecision() {}

//=============================================================================

__attribute__((weak)) int main(int argc, cStringArray argv) {
    static QuakeParms_t	parms;

    parms.memsize = 8 * 1024 * 1024;
    parms.membase = malloc(parms.memsize);
    parms.basedir = ".";

    COM_InitArgv(argc, argv);

    parms.argc = com.argc;
    parms.argv = com.argv;

    printf("Host_Init\n");
    Host_Init(&parms);
    while (1) {
        Host_Frame(0.1);
    }
}


