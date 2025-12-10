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
// sysFS_mcu.c -- system driver to aid porting efforts

#include "sys.h"
// #include "perepherial.h"
// #include <stdio.h>
// #include <string.h>
#include "terminal_tools.h"


/*
===============================================================================

FILE IO

===============================================================================
*/


#include <sys/stat.h>
#if 0
#include "errno.h"
// files
int _open(cStringRO, int, int) { errno = ENOSYS; return -1; }
int _close(int) { errno = ENOSYS; return -1; }
int _lseek(int, int, int) { errno = ENOSYS; return -1; }
int _read(int, cStringArray, int) { errno = ENOSYS; return 0; }
int _fstat(int, struct stat* st) { if (st) st->st_mode = S_IFCHR; return 0; }
int _isatty(int) { return 1; }

#else

#if 0
int _open(char* path, int flags, ...) {
    printf(RED("_open(path:[%s] flags:0x%X);\n"), path, flags);
    (void)path;
    (void)flags;
    /* Pretend like we always fail */
    return -1;
}
#endif

#if 0
int _close(int file) {
    printf(RED("_close(file:%i);\n"), file);
    (void)file;
    return -1;
}
#endif
#if 0
int _lseek(int file, int ptr, int dir) {
    printf(RED("_lseek(file:%i, ptr:%i, dir:%i);\n"), file, ptr, dir);
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}
#endif

#if 0
int _read(int file, char* ptr, int len) {
    printf(RED("_read(file:%i, ptr:%p, len:%i);\n"), file, ptr, len);
    (void)file;

    // for (int DataIdx = 0; DataIdx < len; DataIdx++) {
    //     *ptr++ = __io_getchar();
    // }

    return len;
}
#endif

// int _write(int file, char* ptr, int len) {
//     printf(RED("_write(file:%i, ptr:%p, len:%i);\n"), file, ptr, len);
//     (void)file;
//     int DataIdx;

//     for (DataIdx = 0; DataIdx < len; DataIdx++) {
//         __io_putchar(*ptr++);
//     }
//     return len;
// }



// int _fstat(int file, struct stat* st) {
//     printf(RED("_fstat(file:%i, st:%p);\n"), file, st);
//     (void)file;
//     st->st_mode = S_IFCHR;
//     return 0;
// }

// int _isatty(int file) {
//     printf(RED("_isatty(file:%i);\n"), file);
//     (void)file;
//     return 1;
// }



#endif
