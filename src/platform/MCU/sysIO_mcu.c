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
#include <stdio.h>
#include <stdarg.h>
#include <sys/errno.h>
#include "perepherial.h"


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Printf(cStringRO fmt, ...) {
    va_list argptr;
    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    va_end(argptr);
}

cString Sys_ConsoleInput() {
    return NULL;
}

#if 0
// console write
// Пишите в UART/ITM здесь, если нужно видеть printf
int _write(int, cStringRO buf, int len) { (void)buf; return len; }
#else
int _write(int file, const char *buf, int len) {
    if ((file == 1) || (file == 2)) {
        HAL_StatusTypeDef st = HAL_UART_Transmit(
            &huart1,
            (uint8_p)buf,
            (uint16_t)len,
            HAL_MAX_DELAY
        );

        if (st == HAL_OK) {
            return len;
        } else {
            errno = EIO;
            return -1;
        }
    }

    errno = EBADF;
    return -1;
}
#endif
//=============================================================================