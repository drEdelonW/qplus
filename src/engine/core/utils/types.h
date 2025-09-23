#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "byte_t.h"
#include "qboolean.h"


typedef char* cstring;
typedef const char* cstring_ro;  // read-only
typedef void* typeless_ptr;


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
