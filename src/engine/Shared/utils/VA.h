#pragma once
#include <stdio.h>
#include <stdarg.h>
#include "types.h"

extern va_list argptr;

#define VA_BUFF_SIZE   (1024)
typedef char VaBuff_t[VA_BUFF_SIZE];

#define VA_EXPAND(buf, fmt)                         \
    va_start(argptr, fmt); {                        \
        vsnprintf(buf, sizeof(buf), fmt, argptr);   \
    }va_end(argptr)

#define VA_P_EXPAND(fmt)        \
    va_start(argptr, fmt); {    \
        vprintf(fmt, argptr);   \
    }va_end(argptr)



#ifdef __cplusplus
extern "C" {
#endif

    cString va(cStringRO format, ...); // does a varargs printf into a temp buffer

#ifdef __cplusplus
}
#endif
