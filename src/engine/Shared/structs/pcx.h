#pragma once

#include "types.h"

typedef struct {
    char manufacturer;
    char version;
    char encoding;
    char bits_per_pixel;
    uint16_t xmin, ymin, xmax, ymax;
    uint16_t hres, vres;
    uint8_t palette[48];
    char reserved;
    char color_planes;
    uint16_t bytes_per_line;
    uint16_t palette_type;
    char filler[58];
    uint8_t data;   // unbounded
} pcx_t;
typedef pcx_t* pcx_p;

/*
==============
WritePCXfile
==============
*/
void WritePCXfile(cString filename, uint8_p data, int width, int height, int rowbytes, uint8_p palette);
