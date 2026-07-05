// fixed_math.h
#pragma once
#include "types.h"

// TODO: wrap it to type protected form to avoid mistype operations
// -- Types -------------------------------------------------------------------

// typedef uint16_t    fixed4_t;   // 12.4   unsigned
typedef int16_t     fixed4_t;   // 11.4   signed
typedef fixed4_t*   fixed4_p;

typedef uint16_t    fixed8_t;   // 8.8   usigned
// typedef int16_t     fixed8_t;   // 7.8   signed
typedef fixed8_t*   fixed8_p;

// typedef uint32_t    fixed16_t;  // 16.16 unsigned    // !!not work now!!
typedef int32_t     fixed16_t;  // 15.16 signed
typedef fixed16_t*  fixed16_p;

// -- Constants ----------------------------------------------------------------
#define FIXED4_FRAC_BITS    4
#define FIXED4_ONE          (1 << FIXED4_FRAC_BITS)     // 0x10 (1.0) in 12.4 
#define FIXED4_FRAC_MASK    (FIXED4_ONE - 1)   // 0x0F less then (1.0) in 12.4

#define FIXED8_FRAC_BITS    8
#define FIXED8_ONE          (1 << FIXED8_FRAC_BITS)     // 0x100 (1.0) in 8.8 
#define FIXED8_FRAC_MASK    (FIXED8_ONE - 1)   // 0xFF less then (1.0) in 8.8

#define FIXED16_FRAC_BITS   16
#define FIXED16_ONE         (1 << FIXED16_FRAC_BITS)    // 0x10000 (1.0) in 16.16
#define FIXED16_FRAC_MASK   (FIXED16_ONE - 1)   // 0xFFFF less then (1.0) in 16.16

// -- Conversion ---------------------------------------------------------------
#define FIXED4_TO_INT(x)        ((x) >> FIXED4_FRAC_BITS)
#define INT_TO_FIXED4(x)        ((fixed4_t)(x) << FIXED4_FRAC_BITS)
#define FIXED4_FRAC(x)          ((x) & FIXED4_FRAC_MASK)    // Frac part (x & 0x0F)

#define FIXED8_TO_INT(x)        ((x) >> FIXED8_FRAC_BITS)
#define INT_TO_FIXED8(x)        ((fixed8_t)(x) << FIXED8_FRAC_BITS)
#define FIXED8_FRAC(x)          ((x) & FIXED8_FRAC_MASK)    // Frac part (x & 0xFF)

#define FIXED16_TO_INT(x)       ((x) >> FIXED16_FRAC_BITS)
#define INT_TO_FIXED16(x)       ((fixed16_t)(x) << FIXED16_FRAC_BITS)
#define FIXED16_FRAC(x)         ((x) & FIXED16_FRAC_MASK)   // Frac part (x & 0xFFFF)

// -- Arithmetic ---------------------------------------------------------------
#define FIXED_MID(a, b)         HALF(((a) + (b)))   // midpoint, stays in int

#include "CLAMP.h"
static inline int8_t fixed4_fsat(float f) {
    int v = (int)(f * FIXED4_ONE);
    CLAMP(INT8_MIN, &v, INT8_MAX);
    return (int8_t)v;
}

static inline float fixed4_tof(int8_t v) {
    return (float)v * (1.0f / FIXED4_ONE);
}

static inline fixed8_t fixed8_mul(fixed8_t a, fixed8_t b) {
    return (fixed8_t)(((int32_t)a * b) >> FIXED8_FRAC_BITS);
}

static inline fixed16_t fixed16_mul(fixed16_t a, fixed16_t b) {
    return (fixed16_t)(((int64_t)a * b) >> FIXED16_FRAC_BITS);
}

static inline fixed16_t fixed16_invert24(fixed16_t val) {
    if (val < 256)      return (fixed16_t)0xFFFFFFFF;

    return (fixed16_t)(((double)FIXED16_ONE * (double)0x1000000 / (double)val) + 0.5);
}

#if 0 /* not used */
# if !id386

// TODO: move to nonintel.c

/*
===================
Invert24To16
Inverts an 8.24 value to a 16.16 value
====================
*/

fixed16_t Invert24To16(fixed16_t val);
fixed16_t Invert24To16(fixed16_t val) {
    if (val < 256)      return (0xFFFFFFFF);
    return (fixed16_t)(((double)0x10000 * (double)0x1000000 / (double)val) + 0.5);
}

# endif
#endif