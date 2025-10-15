#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "qboolean.h"


typedef char* cString;
typedef char** cStringArray;
typedef const char* cStringRO;  // read-only
typedef void* TypeLess_ptr;

// Pointers to standard integer types
typedef int8_t*    int8_p;
typedef uint8_t*   uint8_p;
typedef int16_t*   int16_p;
typedef uint16_t*  uint16_p;
typedef int32_t*   int32_p;
typedef uint32_t*  uint32_p;
typedef int64_t*   int64_p;
typedef uint64_t*  uint64_p;

// Pointers to other common types
typedef float*     float_p;
typedef double*    double_p;

#ifndef NULL
#   define NULL    ((TypeLess_ptr)0)
#endif

#if 0
#    define Q_MAXCHAR   ((char)0x7F)
#    define Q_MAXSHORT  ((int16_t)0x7FFF)
#    define Q_MAXINT    ((int)0x7FFFFFFF)
#    define Q_MAXLONG   ((int)0x7FFFFFFF)
#    define Q_MAXFLOAT  ((int)0x7FFFFFFF)

#    define Q_MINCHAR   ((char)0x80)
#    define Q_MINSHORT  ((int16_t)0x8000)
#    define Q_MININT    ((int)0x80000000)
#    define Q_MINLONG   ((int)0x80000000)
#    define Q_MINFLOAT  ((int)0x7FFFFFFF)
#else
#    include <stdint.h>
#    define Q_MAXCHAR   INT8_MAX      //  127
#    define Q_MAXSHORT  INT16_MAX     //  32767
#    define Q_MAXINT    INT32_MAX     //  2147483647
#    define Q_MAXLONG   INT32_MAX     //  совместимо с оригинальным Quake
#    define Q_MAXFLOAT  FLT_MAX       //  3.402823e+38F

#    define Q_MINCHAR   INT8_MIN      // -128
#    define Q_MINSHORT  INT16_MIN     // -32768
#    define Q_MININT    INT32_MIN     // -2147483648
#    define Q_MINLONG   INT32_MIN
#    define Q_MINFLOAT  (-FLT_MAX)
#endif
