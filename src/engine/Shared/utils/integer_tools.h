#pragma once

// -- Bit ops ------------------------------------------------------------------

#define DIV2(x)     ((x) >> 1)  /* fast divide by 2 */
#define MUL2(x)     ((x) << 1)  /* fast multiply by 2 */

#define DIV4(x)     ((x) >> 2)  /* fast divide by 4 */
#define MUL4(x)     ((x) << 2)  /* fast multiply by 4 */

#define DIV8(x)     ((x) >> 3)  /* fast divide by 8 */
#define MUL8(x)     ((x) << 3)  /* fast multiply by 8 */

#define DIV16(x)    ((x) >> 4)  /* fast divide by 16 */
#define MUL16(x)    ((x) << 4)  /* fast multiply by 16 */

#define DIV32(x)    ((x) >> 5)  /* fast divide by 32 */
#define MUL32(x)    ((x) << 5)  /* fast multiply by 32 */

#define DIV64(x)    ((x) >> 6)  /* fast divide by 64 */
#define MUL64(x)    ((x) << 6)  /* fast multiply by 64 */

#define DIV128(x)   ((x) >> 7)  /* fast divide by 128 */
#define MUL128(x)   ((x) << 7)  /* fast multiply by 128 */

#define DIV256(x)   ((x) >> 8)  /* fast divide by 256 */
#define MUL256(x)   ((x) << 8)  /* fast multiply by 256 */

/* aliases */
#define HALF(x)     DIV2(x)
#define TWICE(x)    MUL2(x)

// #define QUARTER(x)  DIV4(x)
// #define QUAD(x)     MUL4(x)

// #define EIGHTH(x)   DIV8(x)
// #define OCTO(x)     MUL8(x)
