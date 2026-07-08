#pragma once

/* Use this macro to place objects into external SDRAM on STM32 target. */
#if defined(STM32) && !defined(_MSC_VER)    /* GCC / Clang for STM32 */
# define PLACE_TO_SDRAM __attribute__((section(".sdram_data"), aligned(4)))
#else   /* Other platforms: ignore, keep default placement */
# define PLACE_TO_SDRAM
#endif

#ifdef __weak
# undef __weak
#endif
#define __weak __attribute__((weak))

/* Portable weak-with-fallback function definition.
 * Plain __attribute__((weak)) on a function *body* only reliably works
 * on ELF. On PE/COFF (mingw) the linker does not treat it as "use this if
 * nothing stronger exists" -- it silently drops it instead (undefined
 * reference), because PE weak symbols require an explicit named default
 * via `alias`, not just a weak body.
 * WEAK_FUNC generates that named default automatically (name + "_Default")
 * and declares a weak alias to it, which is correctly understood by both
 * ELF and PE linkers -- so a strong definition elsewhere still overrides it,
 * and "extinguishing" the strong module falls back to this one.
 * Usage:  WEAK_FUNC(void, VID_Shutdown, (void)) {}
 */
#define WEAK_FUNC(ret, name, params) \
    ret name##_Default params; \
    ret name params __attribute__((weak, alias(#name "_Default"))); \
    ret name##_Default params

