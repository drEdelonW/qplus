// fixed_math.h
#pragma once
#include "types.h"

// -- Types -------------------------------------------------------------------

// typedef int         fixed4_t; // not used
// typedef uint8_t     fixed4_t; // ?

// typedef int         fixed8_t; // DO NOT USE int!!!
typedef uint16_t    fixed8_t;   // 8.8   unsigned
typedef fixed8_t*   fixed8_p;

// typedef int         fixed16_t; // DO NOT USE int!!!
typedef int32_t     fixed16_t;  // 16.16 signed !!!MUST BE SIGNED!!!
// typedef int16_t     fixed16_t; // X
typedef fixed16_t*  fixed16_p;


// -- Constants ----------------------------------------------------------------

#define FIXED16_FRAC_BITS   16
#define FIXED16_ONE         (1 << FIXED16_FRAC_BITS)    // 1.0 в 16.16

#define FIXED8_FRAC_BITS    8
#define FIXED8_ONE          (1 << FIXED8_FRAC_BITS)

// -- Conversion ---------------------------------------------------------------

#define FIXED16_TO_INT(x)       ((x) >> FIXED16_FRAC_BITS)
#define INT_TO_FIXED16(x)       ((fixed16_t)(x) << FIXED16_FRAC_BITS)
#define FIXED16_FRAC(x)         ((x) & (FIXED16_ONE - 1))  // Frac part (x & 0xFFFF)

// -- Bit ops ------------------------------------------------------------------

#define HALF(x)     ((x) >> 1)  /* fast divide by 2 */
#define TWICE(x)    ((x) << 1)  /* fast multiply by 2 */

// -- Arithmetic ---------------------------------------------------------------

#define FIXED16_MID(a, b)       HALF(((a) + (b)))         // midpoint, stays in 16.16

static inline fixed16_t fixed16_mul(fixed16_t a, fixed16_t b) {
    return (fixed16_t)(((int64_t)a * b) >> FIXED16_FRAC_BITS);
}

static inline fixed16_t fixed16_invert24(fixed16_t val) {
    if (val < 256) return (fixed16_t)0xFFFFFFFF;
    return (fixed16_t)(((double)FIXED16_ONE * (double)0x1000000 / (double)val) + 0.5);
}

// fixed16_t Invert24To16(fixed16_t val);

#if !id386

// TODO: move to nonintel.c

// /*
// ===================
// Invert24To16
// Inverts an 8.24 value to a 16.16 value
// ====================
// */

// fixed16_t Invert24To16(fixed16_t val) {
//     if (val < 256)      return (0xFFFFFFFF);
//     return (fixed16_t)(((double)0x10000 * (double)0x1000000 / (double)val) + 0.5);
// }

#endif