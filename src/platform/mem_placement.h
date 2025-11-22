#pragma once
// #ifndef MEM_PLACEMENT_H
// #define MEM_PLACEMENT_H

/* Use this macro to place objects into external SDRAM on STM32 target. */
#if defined(STM32) && !defined(_MSC_VER)
/* GCC / Clang for STM32 */
#define PLACE_TO_SDRAM __attribute__((section(".sdram_data"), aligned(4)))
#else
/* Other platforms: ignore, keep default placement */
#define PLACE_TO_SDRAM
#endif

#define WEAK __attribute__((weak))
// #endif /* MEM_PLACEMENT_H */

