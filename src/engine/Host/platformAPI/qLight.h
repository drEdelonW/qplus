#pragma once
#include "qColor.h"

#if 1 /* =====================[ colorMap ]=====================*/

#define VID_CBITS 6
#define VID_GRADES (1 << VID_CBITS) /* 64 Illumination Levels Num */
#define ColormapDIM (VID_GRADES * InksNum)  /* 64 * 256 = 16384 */

typedef struct {
    union {
        palMap_t    illum[VID_GRADES];
        qColor8_t   raw[ColormapDIM];
    };
} ColorMap_t;
typedef ColorMap_t* ColorMap_p;

#endif /* ====================={ colorMap end }=====================*/


extern ColorMap_p host_colormap;
