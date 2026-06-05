#include "pcx.h"
#include "endian_tools.h"
#include "q_tools.h"
#include "z_hunk.h"
#include "console.h"
#include "common.h"

/*
==============
WritePCXfile
==============
*/
void WritePCXfile(cString filename, uint8_p data, int width, int height, int rowbytes, uint8_p palette) {
    pcx_p pcx = Hunk_TempAlloc(width * height * 2 + 1000);
    if (pcx == NULL) {
        Con_Printf("SCR_ScreenShot_f: not enough memory\n");
        return;
    }

    pcx->manufacturer = 0x0a; // PCX id
    pcx->version = 5;   // 256 color
    pcx->encoding = 1;  // uncompressed
    pcx->bits_per_pixel = 8;  // 256 color
    pcx->xmin = 0;
    pcx->ymin = 0;
    pcx->xmax = LittleShort((int16_t)(width - 1));
    pcx->ymax = LittleShort((int16_t)(height - 1));
    pcx->hres = LittleShort((int16_t)width);
    pcx->vres = LittleShort((int16_t)height);
    Q_memset(pcx->palette, 0, sizeof(pcx->palette));
    pcx->color_planes = 1;  // chunky image
    pcx->bytes_per_line = LittleShort((int16_t)width);
    pcx->palette_type = LittleShort(2);  // not a grey scale
    Q_memset(pcx->filler, 0, sizeof(pcx->filler));

    // pack the image
    uint8_p pack = &pcx->data;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if ((*data & 0xC0) != 0xC0)
                *pack++ = *data++;
            else {
                *pack++ = 0xc1;
                *pack++ = *data++;
            }
        }

        data += rowbytes - width;
    }

    // write the palette
    *pack++ = 0x0C; // palette ID uint8_t
    for (int i = 0; i < 768; i++)
        *pack++ = *palette++;

    // write output file
    COM_WriteFile(filename, pcx, pack - (uint8_p)pcx);
}

