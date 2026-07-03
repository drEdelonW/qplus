#include "tga.h"
#include <stdlib.h>
#include <string.h>
#include "host.h"
#include "common.h"
#include "console.h"

typedef struct __attribute__((packed)) {
    uint8_t     id_length;
    uint8_t     colormap_type;
    uint8_t     image_type;
    uint16_t    colormap_index;
    uint16_t    colormap_length;
    uint8_t     colormap_size;
    uint16_t    x_origin;
    uint16_t    y_origin;
    uint16_t    width;
    uint16_t    height;
    uint8_t     pixel_size;
    uint8_t     attributes;
} TargaHeader_t;
typedef  TargaHeader_t* TargaHeader_p;

uint16_t fgetLittleShort(FILE* f) {
    uint8_t b0 = fgetc(f);
    uint8_t b1 = fgetc(f);

    return (uint16_t)(
        (b0 << 0) |
        (b1 << 8)
        );
}

#if 0 /* not used */
uint32_t fgetLittleLong(FILE* f) {
    uint8_t b0 = fgetc(f);
    uint8_t b1 = fgetc(f);
    uint8_t b2 = fgetc(f);
    uint8_t b3 = fgetc(f);

    return (uint32_t)(
        (b0 << 0) |
        (b1 << 8) |
        (b2 << 16) |
        (b3 << 24)
        );
}
#endif

/*
=============
LoadTGA
=============
*/
uint8_p LoadTGA(FILE* fin) {
    TargaHeader_t hdr = (TargaHeader_t){
        .id_length          = fgetc(fin),
        .colormap_type      = fgetc(fin),
        .image_type         = fgetc(fin),
        .colormap_index     = fgetLittleShort(fin),
        .colormap_length    = fgetLittleShort(fin),
        .colormap_size      = fgetc(fin),
        .x_origin           = fgetLittleShort(fin),
        .y_origin           = fgetLittleShort(fin),
        .width              = fgetLittleShort(fin),
        .height             = fgetLittleShort(fin),
        .pixel_size         = fgetc(fin),
        .attributes         = fgetc(fin)
    };

    if ((hdr.image_type != 2) &&
        (hdr.image_type != 10)
        )   Host_SysError("LoadTGA: Only type 2 and 10 targa RGB images supported\n");

    if ((hdr.colormap_type != 0) ||
        (
            (hdr.pixel_size != 32) &&
            (hdr.pixel_size != 24))
        )   Host_SysError("Texture_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

    int columns = hdr.width;
    int rows = hdr.height;
    int numPixels = columns * rows;

    uint8_p targa_rgba = malloc(numPixels * 4);

    if (hdr.id_length != 0)
        fseek(fin, hdr.id_length, SEEK_CUR);  // skip TARGA image comment

    struct {
        uint8_t R, G, B, A;
    } px;

    if (hdr.image_type == 2) {  // Uncompressed, RGB images
        for (int row = rows - 1; row >= 0; row--) {
            uint8_p pixbuf = targa_rgba + (row * columns * 4);
            for (int column = 0; column < columns; column++) {
                switch (hdr.pixel_size) {
                case 24: { px.B = getc(fin);   px.G = getc(fin);   px.R = getc(fin);   px.A = 255; } break;
                case 32: { px.B = getc(fin);   px.G = getc(fin);   px.R = getc(fin);   px.A = getc(fin); } break;
                default: Host_Error("pixel_size [0x%X] not supported\n", hdr.pixel_size);
                }
                *pixbuf++ = px.R;   *pixbuf++ = px.G;   *pixbuf++ = px.B;   *pixbuf++ = px.A;
            }
        }
    }
    else if (hdr.image_type == 10) {   // Runlength encoded RGB images
        for (int row = rows - 1; row >= 0; row--) {
            uint8_p pixbuf = targa_rgba + (row * columns * 4);
            for (int column = 0; column < columns; ) {
                int packetHeader = getc(fin);
                int packetSize = 1 + (packetHeader & 0x7f);

                if (packetHeader & 0x80) {        // run-length packet
                    switch (hdr.pixel_size) {
                    case 24: { px.B = getc(fin);    px.G = getc(fin);   px.R = getc(fin);   px.A = 255; } break;
                    case 32: { px.B = getc(fin);    px.G = getc(fin);   px.R = getc(fin);   px.A = getc(fin); } break;
                    default: Host_Error("pixel_size [0x%X] not supported\n", hdr.pixel_size);
                    }

                    for (int j = 0; j < packetSize; j++) {
                        *pixbuf++ = px.R;   *pixbuf++ = px.G;   *pixbuf++ = px.B;   *pixbuf++ = px.A;
                        column++;
                        if (column == columns) { // run spans across rows
                            column = 0;
                            if (row > 0)    row--;
                            else            goto breakOut;
                            pixbuf = targa_rgba + (row * columns * 4);
                        }
                    }
                }
                else {      // non run-length packet
                    for (int j = 0; j < packetSize; j++) {
                        switch (hdr.pixel_size) {
                        case 24: { px.B = getc(fin);   px.G = getc(fin);   px.R = getc(fin);   px.A = 255; } break;
                        case 32: { px.B = getc(fin);   px.G = getc(fin);   px.R = getc(fin);   px.A = getc(fin); } break;
                        default: Host_Error("pixel_size [0x%X] not supported\n", hdr.pixel_size);
                        }
                        *pixbuf++ = px.R;   *pixbuf++ = px.G;   *pixbuf++ = px.B;   *pixbuf++ = px.A;

                        column++;
                        if (column == columns) { // pixel packet run spans across rows
                            column = 0;
                            if (row > 0)    row--;
                            else            goto breakOut;
                            pixbuf = targa_rgba + (row * columns * 4);
                        }
                    }
                }
            }
        breakOut:;
        }
    }

    fclose(fin);
    return targa_rgba;
}


/*
==============
WriteTGAfile
==============
*/
void WriteTGAfile(
    cString filename, uint8_p data,
    int width, int height,
    int rowbytes, uint8_p palette
) {
#if 0
    int hdSz = 18;
#else
    int hdSz = sizeof(TargaHeader_t);
#endif
    if (palette) {
        /* not support yet */
        Host_Error("WriteTGAfile: palette mode not supported\n");
    }
    else {
        int pxSz = 3;
        int buffSz = width * height * pxSz;
        int fileSz = hdSz + buffSz;

        uint8_p tga = malloc(fileSz); {
#if 0
            memset(tga, 0, hdSz);
            tga[2] = 2;   // uncompressed type
            tga[12] = width & 0xFF;   tga[13] = width >> 8;
            tga[14] = height & 0xFF;  tga[15] = height >> 8;
            tga[16] = MUL8(pxSz);    // pixel size (24)
#else
            * (TargaHeader_p)tga = (TargaHeader_t){
                .image_type = 2,            // uncompressed type
                .width = (uint16_t)width,
                .height = (uint16_t)height,
                .pixel_size = MUL8(pxSz),   // pixel size (24)
        };
#endif
            memcpy((uint8_p)tga + (ptrdiff_t)hdSz, data, buffSz);

            // swap rgb to bgr
            for (int i = hdSz; i < (hdSz + buffSz); i += pxSz) {
                uint8_t temp = tga[i];
                tga[i] = tga[i + 2];
                tga[i + 2] = temp;
            }
            COM_WriteFile(filename, tga, fileSz);
    } free(tga);
}
}