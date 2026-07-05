// fixed_math.h
#pragma once
#include "types.h"

// TODO: wrap it to type protected form to avoid mistype operations
// -- Types -------------------------------------------------------------------

// -- fixed4_t (11.4 signed) ----------------------------------------------
#if 1 /* old typeless alias — kept for reference, do not use directly */
// typedef uint16_t    fixed4_t;   // 12.4   unsigned
typedef int16_t     fixed4_t;   // 11.4   signed
#else
typedef struct {
    int16_t fx;     // 11.4   signed
} fixed4_t;
#endif
typedef fixed4_t*   fixed4_p;

// -- fixed8_t (8.8 unsigned) ----------------------------------------------
#if 1 /* old typeless alias — kept for reference, do not use directly */
typedef uint16_t    fixed8_t;   // 8.8   usigned
// typedef int16_t     fixed8_t;   // 7.8   signed
#else
typedef struct {
    uint16_t fx;    // 8.8    unsigned
} fixed8_t;
#endif
typedef fixed8_t*   fixed8_p;

// -- fixed16_t (15.16 signed) ----------------------------------------------
#if 1 /* old typeless alias — kept for reference, do not use directly */
// typedef uint32_t    fixed16_t;  // 16.16 unsigned    // !!not work now!!
typedef int32_t     fixed16_t;  // 15.16 signed
#else
typedef struct {
    int32_t fx;     // 15.16  signed
} fixed16_t;
#endif
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

// -- fixed4_t (11.4 signed) --
static inline int      Fx4ToInt(fixed4_t v)   { return v >> FIXED4_FRAC_BITS; }
static inline fixed4_t IntToFx4(int v)        { return (fixed4_t)(v << FIXED4_FRAC_BITS); }
static inline fixed4_t Fx4Frac(fixed4_t v)    { return v & FIXED4_FRAC_MASK; }
static inline float    Fx4ToFl(fixed4_t v)    { return (float)v * (1.0f / FIXED4_ONE); }
static inline fixed4_t FlToFx4(float f)       { return (fixed4_t)(f * FIXED4_ONE); }

// -- fixed8_t (8.8 unsigned) --
static inline int      Fx8ToInt(fixed8_t v)   { return v >> FIXED8_FRAC_BITS; }
static inline fixed8_t IntToFx8(int v)        { return (fixed8_t)(v << FIXED8_FRAC_BITS); }
static inline fixed8_t Fx8Frac(fixed8_t v)    { return v & FIXED8_FRAC_MASK; }
static inline float    Fx8ToFl(fixed8_t v)    { return (float)v * (1.0f / FIXED8_ONE); }
static inline fixed8_t FlToFx8(float f)       { return (fixed8_t)(f * FIXED8_ONE); }

// -- fixed16_t (15.16 signed) --
static inline int       Fx16ToInt(fixed16_t v)   { return v >> FIXED16_FRAC_BITS; }
static inline fixed16_t IntToFx16(int v)         { return (fixed16_t)((int32_t)v << FIXED16_FRAC_BITS); }
static inline fixed16_t Fx16Frac(fixed16_t v)    { return v & FIXED16_FRAC_MASK; }
static inline float     Fx16ToFl(fixed16_t v)    { return (float)v * (1.0f / FIXED16_ONE); }
static inline fixed16_t FlToFx16(float f)        { return (fixed16_t)(f * FIXED16_ONE); }

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