#pragma once
#include <stdio.h>
#include <stdarg.h>
#include "types.h"

extern va_list argptr;

#define VA_EXPAND(buf, fmt)                         \
    va_start(argptr, fmt); {                        \
        vsnprintf(buf, sizeof(buf), fmt, argptr);   \
    }va_end(argptr) 

#ifdef __cplusplus
extern "C" {
#endif
    // does a varargs printf into a temp buffer
    cString va(cStringRO format, ...); 

#ifdef __cplusplus
}
#endif
