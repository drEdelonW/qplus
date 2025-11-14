#pragma once

#include "types.h"

typedef struct {
    int8_t      manufacturer;
    int8_t      version;
    int8_t      encoding;
    int8_t      bits_per_pixel;
    uint16_t    xmin, ymin, xmax, ymax;
    uint16_t    hres, vres;
    uint8_t     palette[48];
    int8_t      reserved;
    int8_t      color_planes;
    uint16_t    bytes_per_line;
    uint16_t    palette_type;
    int8_t      filler[58];
    uint8_t     data;   // unbounded
} pcx_t;
typedef pcx_t* pcx_p;

/*
==============
WritePCXfile
==============
*/
void WritePCXfile(cString filename, uint8_p data, int width, int height, int rowbytes, uint8_p palette);
