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
#include "errno.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "sys.h"
#include "host.h"


/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 10
FILE* sys_handles[MAX_HANDLES];

int findhandle() {
    for (int i = 1; i < MAX_HANDLES; i++)
        if (!sys_handles[i])
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

int Sys_FileOpenRead(cString path, int* hndl) {
    int i = findhandle();
    FILE* f = fopen(path, "rb");
    if (!f) {
        *hndl = -1;
        return -1;
    }
    sys_handles[i] = f;
    *hndl = i;

    return filelength(f);
}

int Sys_FileOpenWrite(cString path) {
    int i = findhandle();
    FILE* f = fopen(path, "wb");
    if (!f)
        Sys_Error("Error opening %s: %s", path, strerror(errno));
    sys_handles[i] = f;

    return i;
}

void Sys_FileClose(int handle) {
    fclose(sys_handles[handle]);
    sys_handles[handle] = NULL;
}

void Sys_FileSeek(int handle, int position) {
    fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, TypeLess_ptr dest, int count) {
    return fread(dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite(int handle, TypeLess_ptr data, int count) {
    return fwrite(data, 1, count, sys_handles[handle]);
}

int Sys_FileTime(cString path) {
    FILE* f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return 1;
    }
    return -1;
}

void Sys_mkdir(cString path) {}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable(uintptr_t startaddr, size_t length) {}

void Sys_Error(cStringRO error, ...) {
    printf("Sys_Error: ");
    va_list argptr;
    va_start(argptr, error);
    vprintf(error, argptr);
    va_end(argptr);
    printf("\n");

    exit(1);
}

void Sys_Printf(cString fmt, ...) {
    va_list argptr;
    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    va_end(argptr);
}

void Sys_Quit() {
    exit(0);
}

double Sys_FloatTime() {
    static double t;
    t += 0.1;
    return t;
}

cString Sys_ConsoleInput() {
    return NULL;
}

void Sys_Sleep() {}
void Sys_SendKeyEvents() {}
void Sys_HighFPPrecision() {}
void Sys_LowFPPrecision() {}

#if 0
#include <sys/stat.h>
// files
int _close(int) { errno = ENOSYS; return -1; }
int _fstat(int, struct stat* st) { if (st) st->st_mode = S_IFCHR; return 0; }
int _isatty(int) { return 1; }
int _lseek(int, int, int) { errno = ENOSYS; return -1; }
int _read(int, cStringArray, int) { errno = ENOSYS; return 0; }
int _unlink(cStringRO) { errno = ENOSYS; return -1; }
int _open(cStringRO, int, int) { errno = ENOSYS; return -1; }

// procs
int _kill(int, int) { errno = ENOSYS; return -1; }
int _getpid(void) { return 1; }

// memory
static cStringArray heap_end;
void* _sbrk(ptrdiff_t incr) { if (!heap_end) heap_end = &_end; cStringArray p = heap_end; heap_end += incr; return p; }

// console write
// Пишите в UART/ITM здесь, если нужно видеть printf
int _write(int, cStringRO buf, int len) { (void)buf; return len; }
#endif
//=============================================================================

// void main(int argc, cStringArray argv) {
// int main() {
//     static QuakeParms_t parms;
//     int argc;
//     cStringArray argv;
//     parms.memsize = 8 * 1024 * 1024;
//     parms.membase = malloc(parms.memsize);
//     parms.baseDir = ".";

//     COM_InitArgv(argc, argv);

//     parms.argc = com.argc;
//     parms.argv = com.argv;

//     printf("Host_Init\n");
//     Host_Init(&parms);
//     while (1) {
//         Host_Frame(0.1);
//     }
// }


