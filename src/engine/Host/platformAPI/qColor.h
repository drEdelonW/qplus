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



// !!! must be kept the same as in quakeasm.h !!!
typedef enum {
    InkConTransp    = 0x00,  /* console symbol transparent color */
    GRAPH_BG        = 0x30,  // background color
    GRAPH_FG        = 0xFF,  // bright bar color
    InkTransp       = 0xFF,  /* texture transparent color */
    InksNum         = 256,   /* number of colors in palette space */
} InkIdx_t;

typedef struct {
    uint8_t i;  // InkIdx_t but uint8_t size
} qColor8_t;    STATIC_ASSERT_SIZE(qColor8_t, 1); // 1
typedef qColor8_t* qColor8_p;

#if 1 /* =====================[ Palette ]=====================*/
#define PalRawDIM   (InksNum * RGB_DIM) /* 256 * 3 = 768 */

typedef struct {
    qColor8_t pal[InksNum];
} palMap_t;
typedef palMap_t* palMap_p;

typedef struct {
    union {
        qRgb24  ink[InksNum];
        uint8_t raw[PalRawDIM];
    };
} qPal_t;      STATIC_ASSERT_SIZE(qPal_t, 256*3); // 768
typedef qPal_t* qPal_p;
#endif /* ====================={ Palette end }=====================*/

extern qPal_p   host_basepal;
extern Rgb16_t  d_8to16table[InksNum];  // not used in 8 bpp mode
extern Rgb24_t  d_8to24table[InksNum];  // not used in 8 bpp mode // 0xAABBGGRR
extern int  r_pixbytes; // TODO: move to SoftRender specific area

#ifdef __cplusplus
extern "C" {
#endif

    void    VID_ShiftPalette(qPal_p palette);   // called for bonus and pain flashes, and for underwater color changes
    void    VID_SetPalette(qPal_p palette);     // called at startup and after any gamma correction

#ifdef __cplusplus
}
#endif