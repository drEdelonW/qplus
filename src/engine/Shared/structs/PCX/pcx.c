#include "pcx.h"
#include <stdlib.h>
#include "endian_tools.h"
#include "q_tools.h"
#include "z_hunk.h"
#include "console.h"
#include "common.h"


enum {
    PCXmagic        = 0x0A, // PCX magic: 0x0A
    PaletteMarker   = 0x0C, // palette ID
    RleFlag         = 0xC0, // top 2 bits set = run marker
    RleLenMask      = 0x3F, // bottom 6 bits = run length (1..63)
};

typedef struct {
    uint8_t     manufacturer;       // PCX magic: 0x0A
    uint8_t     version;            // 0/2/3/4/5
    uint8_t     encoding;           // 1 = RLE
    uint8_t     bits_per_pixel;     // per plane: 1/2/4/8
    uint16_t    xmin;
    uint16_t    ymin;
    uint16_t    xmax;
    uint16_t    ymax;
    uint16_t    hres;
    uint16_t    vres;
    uint8_t     palette[48];
    uint8_t     reserved;
    uint8_t     color_planes;       // 1..4
    uint16_t    bytes_per_line;
    uint16_t    palette_type;       // 1=color, 2=greyscale
    uint8_t     filler[58];
    uint8_t     data;               // unbounded
} pcx_t;
typedef pcx_t* pcx_p;




/*
============
LoadPCX
============
*/
uint8_p LoadPCX(FILE* f_pcx) {
    //
    // parse the PCX file
    //
    pcx_t pcx; fread(&pcx, 1, sizeof(pcx), f_pcx);
    if ((pcx.manufacturer   != PCXmagic) ||
        (pcx.version        != 5) ||
        (pcx.encoding       != 1) ||    // compressed RLE
        (pcx.bits_per_pixel != 8) ||    // 256 color
        (pcx.xmax           >= 320) ||
        (pcx.ymax           >= 256)
        ) {
        Con_Printf("Bad pcx file\n");
        return NULL;
    }

    // seek to palette
    uint8_t palette[768];
    fseek(f_pcx, -768, SEEK_END); fread(palette, 1, 768, f_pcx);

    uint16_t width = pcx.xmax + 1;
    uint16_t height = pcx.ymax + 1;
    int count = width * height;
    uint8_p pcx_rgb = malloc(QUAD(count));
    
    fseek(f_pcx, offsetof(pcx_t, data), SEEK_SET);
    for (int y = 0; y < height; y++) {
        uint8_p pix = pcx_rgb + QUAD(y * width);
        for (int x = 0; x < width; ) {
            int token = fgetc(f_pcx);
            bool isRle = (token & RleFlag) == RleFlag;

            int len  = isRle ? (token & RleLenMask) : 1;
            int cIdx = isRle ? fgetc(f_pcx) : token;

            while (len-- > 0) {
                pix[0] = palette[cIdx * 3 + 0];
                pix[1] = palette[cIdx * 3 + 1];
                pix[2] = palette[cIdx * 3 + 2];
                pix[3] = 255;
                pix += 4;
                x++;
            }
        }
    }
    return pcx_rgb;
}


/*
==============
WritePCXfile
==============
*/
void WritePCXfile(
    cString filename, uint8_p data,
    int width, int height,
    int rowbytes, uint8_p palette
) {
    pcx_p pcx = Hunk_TempAlloc(TWICE(width * height) + 1000);
    if (pcx == NULL) {
        Con_Printf("SCR_ScreenShot_f: not enough memory\n");
        return;
    }

    *pcx = (pcx_t){
        .manufacturer   = PCXmagic, // PCX id
        .version        = 5,        // 256 color
        .encoding       = 1,        // compressed RLE
        .bits_per_pixel = 8,        // 256 color
        // .xmin = 0,
        // .ymin = 0,
        .xmax = LittleShort((int16_t)(width - 1)),
        .ymax = LittleShort((int16_t)(height - 1)),
        .hres = LittleShort((int16_t)width),
        .vres = LittleShort((int16_t)height),
        // .palette = {0},
        .color_planes   = 1,        // chunky image
        .bytes_per_line = LittleShort((int16_t)width),
        .palette_type   = LittleShort(2), // not a grey scale
        // .filler {0},
    };

    // pack the image
    uint8_p pack = &pcx->data;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if ((*data & RleFlag) == RleFlag)
                *pack++ = RleFlag | 1;
            *pack++ = *data++;
        }
        data += rowbytes - width;
    }

    // write the palette
    *pack++ = PaletteMarker; // palette ID
    for (int i = 0; i < 768; i++)
        *pack++ = *palette++;

    // write output file
    COM_WriteFile(filename, pcx, pack - (uint8_p)pcx);
}
