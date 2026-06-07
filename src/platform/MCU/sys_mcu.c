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
#include <stdlib.h>
#include <stdarg.h>
#include "perepherial.h"
#include "common.h"
#include "host.h"
#include "versions.h"


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

void Sys_Quit() {
    exit(0);
}

void Sys_Sleep() {}
void Sys_HighFPPrecision() {}
void Sys_LowFPPrecision() {}

LegacyTimeStamp_t Sys_FloatTime() {
    static LegacyTimeStamp_t t = 0.0;
    t += 0.1;
    return t;
}

#if 0

// procs
int _kill(int, int) { errno = ENOSYS; return -1; }
int _getpid() { return 1; }

// memory
static cStringArray heap_end;
void* _sbrk(ptrdiff_t incr) { if (!heap_end) heap_end = &_end; cStringArray p = heap_end; heap_end += incr; return p; }


#endif
//=============================================================================

// void main(int argc, cStringArray argv) {
int main() {
    CoreClock_Init();
    Pereph_Init();
    printf("\n[MCU Init ok]\n\n");

    size_t  memsize = 8 * 1024 * 1024;
    int argc = 0;
    cStringArray argv = 0;

    COM_InitArgv(argc, argv);

    static QuakeParms_t parms;
    parms.baseDir = ".";
    parms.argc = com.argc;
    parms.argv = com.argv;
    parms.membase = malloc(memsize);
    parms.memsize = memsize;

    while (!parms.membase) { printf("NO MEMORY\n"); }

    printf("Host_Init\n");
    Host_Init(&parms);
    printf("STM32 Quake -- Version %0.3f\n", STM32_VERSION);

    LegacyTimeStamp_t oldtime = Sys_FloatTime();
    while (1) {
        LegacyTimeStamp_t newtime = Sys_FloatTime();
        LegacyTimeStamp_t time = newtime - oldtime;
        Host_Frame(time);
        oldtime = newtime;
    }
}


#include <stdint.h>
#include <errno.h>

extern uint8_t __sdram_heap_start;
extern uint8_t __sdram_heap_end;

static uint8_t* heap_ptr = NULL;

void* _sbrk(ptrdiff_t incr) {
    if (heap_ptr == NULL)
        heap_ptr = &__sdram_heap_start;

    uint8_t* prev = heap_ptr;
    uint8_t* next = heap_ptr + incr;

    if (next >= &__sdram_heap_end) {
        errno = ENOMEM;
        return (void*)-1;
    }

    heap_ptr = next;
    return prev;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM6) {
        HAL_IncTick();
    }
}

void Error_Handler() {
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {}
}