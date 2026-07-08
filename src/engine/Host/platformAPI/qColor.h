#pragma once
#include "types.h"
#include "assert.h"


typedef uint16_t Rgb16_t;
typedef Rgb16_t* Rgb16_p;
#if 1 /* TODO: deprecated naming, migrate call sites to Rgb16_t/Rgb16_p, then drop */
typedef Rgb16_t PIXEL16;
typedef Rgb16_p PIXEL16_p;
#endif


typedef uint32_t Rgb24_t;
typedef Rgb24_t* Rgb24_p;
#if 1 /* TODO: deprecated naming, migrate call sites to Rgb24_t/Rgb24_p, then drop */
typedef Rgb24_t PIXEL24;
typedef Rgb24_p PIXEL24_p;
#endif


#if 1
typedef uint32_t Rgb32_t;
#else
typedef union {
    struct { uint8_t r, g, b, a; } rgba;
    struct { uint8_t a, b, g, r; } abgr;
    struct { uint8_t b, g, r, a; } bgra;
    struct { uint8_t a, r, g, b; } argb;
    uint8_t  raw[4];
} Rgb32_t;
#endif
typedef Rgb32_t* Rgb32_p;


typedef enum {
    R_CH = 0u,
    G_CH = 1u,
    B_CH = 2u,
    RGB_DIM = 3u,
} rgbCh_t;

typedef struct {
    union {
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        };
        uint8_t ch[RGB_DIM];
    };
} qRgb24;   // Quake specific 24bit color

#if 0
typedef uint8_t pixel_t;
#else
typedef struct {
    uint16_t c;
} qColor16_t;
#endif
typedef qColor16_t* qColor16_p;


#if 0
typedef uint8_t pixel_t;
#else
typedef struct {
    uint8_t i;
} qColor8_t;
#endif
typedef qColor8_t* qColor8_p;
// !!! must be kept the same as in quakeasm.h !!!

#if 1 /* =====================[ Palette ]=====================*/
#define TRANSPARENT_COLOR (0xFF)
#define InksNum     (256)               /* number of colors in palette */
#define PalRawDIM   (InksNum * RGB_DIM) /* 256 * 3 = 768 */

typedef struct {
    union {
        qRgb24  ink[InksNum];
        uint8_t raw[PalRawDIM];
    };
} palette_t;      STATIC_ASSERT_SIZE(palette_t, 256*3); // 768
typedef palette_t* palette_p;
#endif /* ====================={ Palette end }=====================*/

