#include <stdio.h>
#include "terminal_tools.h"
#include "vid.h"

#include "stm32f769i_discovery.h"
#include "stm32f769i_discovery_lcd.h"
#include "mem_placement.h"

#define LCD_SCREEN_WIDTH              800
#define LCD_SCREEN_HEIGHT             480
#define ARGB8888_BYTE_PER_PIXEL       4

#if 1
// uint8_t LCD_FG_LAYER_ADDRESS[LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT * ARGB8888_BYTE_PER_PIXEL] PLACE_TO_SDRAM;
// uint32_t LCD_FG_LAYER_ADDRESS[LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT] PLACE_TO_SDRAM;
// uint8_t LCD_BG_LAYER_ADDRESS[LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT * ARGB8888_BYTE_PER_PIXEL] PLACE_TO_SDRAM;
uint32_t LCD_BG_LAYER_ADDRESS[LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT] PLACE_TO_SDRAM;
#else
/* LTDC foreground layer address 800x480 in ARGB8888 */
#define LCD_FG_LAYER_ADDRESS          LCD_FB_START_ADDRESS
/* LTDC background layer address 800x480 in ARGB8888 following the foreground layer */
#define LCD_BG_LAYER_ADDRESS          LCD_FG_LAYER_ADDRESS + (LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT * ARGB8888_BYTE_PER_PIXEL)

// #define INTERNAL_BUFFER_START_ADDRESS LCD_BG_LAYER_ADDRESS + (LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT * ARGB8888_BYTE_PER_PIXEL)
#endif
extern DSI_HandleTypeDef hdsi_discovery;
extern LTDC_HandleTypeDef  hltdc_discovery;

/* Stop LCD streaming to free SDRAM / AXI bus */
void LCD_BusPause() {
    HAL_DSI_Stop(&hdsi_discovery);
    __HAL_LTDC_DISABLE(&hltdc_discovery);
}

/* Resume LCD streaming after heavy SD / FS operations */
void LCD_BusResume() {
    __HAL_LTDC_ENABLE(&hltdc_discovery);
    HAL_DSI_Start(&hdsi_discovery);
}
void LCD_Init() {
    // printf(RED("LCD_Init\n"));

    /*##-1- Configure LCD ######################################################*/
       /* LCD DSI initialization in mode Video Burst */
    /* Initialize DSI LCD */
    BSP_LCD_Init();
    BSP_LCD_Clear(Color_Black); BSP_LCD_Clear(Color_Black); BSP_LCD_Clear(Color_Black);

    // BSP_LCD_LayerDefaultInit(background, LCD_FB_START_ADDRESS);
    // BSP_LCD_SetTransparency(background, 0x80);
    // BSP_LCD_SelectLayer(background);    /* Select the LCD Background Layer */
    // BSP_LCD_Clear(Color_Blue);          /* Clear the Background Layer */

    BSP_LCD_LayerDefaultInit(foreground, (uint32_t)LCD_BG_LAYER_ADDRESS);
    BSP_LCD_SetTransparency(foreground, 0xFF);
    BSP_LCD_SelectLayer(foreground);    /* Select the LCD Foreground Layer */
    BSP_LCD_Clear(Color_Black);  /* Clear the Foreground Layer */

    /* Configure the transparency for foreground and background : Increase the transparency */
    /*##-1- Configure LCD END ######################################################*/


    // BSP_LCD_SetBrightness(0xFF);
    // BSP_LCD_SetTextColor(Color_White);
    // BSP_LCD_SetBackColor(Color_Black);
    // BSP_LCD_SetFont(&Font24);
    // BSP_LCD_DisplayStringAtLine(0, (uint8_t*)"!!!LCD WORK!!!");
    // BSP_LCD_DisplayStringAt(0, 0, "!!!LCD WORK!!!", CENTER_MODE);
    // BSP_LCD_SetFont(&Font20);
    // BSP_LCD_DisplayStringAtLine(4, "!!!LCD WORK!!!");
    // BSP_LCD_DisplayStringAt(0, 440, "!!!LCD WORK!!!", CENTER_MODE);
    // int y = 480;
#if 1
    // for (int i = 0; i < y; i++) {
    //     BSP_LCD_DrawPixel(i, i, Color_Black);
    //     BSP_LCD_DrawPixel(i + y, y - i, Color_Black);
    // }
#else
    BSP_LCD_SetTextColor(Color_Black);
    BSP_LCD_DrawLine(0, 0, y, y);
    BSP_LCD_DrawLine(y + y, 0, y, y);
#endif
    // LCD_BusPause();
    // while (1) { ; }

}
static uint32_t _phase = 0; // free-running frame counter, sliced below per refresh mode
void VID_Update(vRect_p rects) {
    if (rects) {
        if ((rects->x != 0) ||
            (rects->y != 0) ||
            (rects->width != 320) ||
            ((rects->height != 200) && (rects->height != 152))
            ) {
            printf(RED("rect:\n x:%i/%i\n y:%i/%i\n"),
                rects->x, rects->width,
                rects->y, rects->height
            );
            return;
        }
    }
    _phase++;

#if 0   /* quarter refresh: one of the 4 sub-pixels per frame */
    int dx = _phase & 1;
    int dy = (_phase >> 1) & 1;
#else   /* checkerboard refresh: one diagonal (2 of 4 sub-pixels) per frame */
    bool cur = _phase & 1;
#endif

    int ofs = ((800 - 640) / 2) + (((480 - 400) / 2) * LCD_SCREEN_WIDTH);
    for (int y = 0; y < Scr.vrect.height; y++)
        for (int x = 0; x < Scr.vrect.width; x++) {
            uint8_t idx = Scr.vrect.pBuff[y * Scr.vrect.width + x];

            uint32_t rgb = d_8to24table[idx];   // 0x00_bb_gg_rr

            uint32_t argb =
                Color_Transparent |              // alpha
                ((rgb & 0x00FF00) << 0 ) |       // g stays put
                ((rgb & 0x0000FF) << 16) |       // r -> bits 16-23
                ((rgb & 0xFF0000) >> 16);        // b -> bits 0-7

            int base = ofs + TWICE(x) + (TWICE(y) * LCD_SCREEN_WIDTH);

#if 0   /* quarter refresh */
            LCD_BG_LAYER_ADDRESS[base + dx + (dy * LCD_SCREEN_WIDTH)] = argb;
#elif 0   /* checkerboard refresh */
            if (cur) {
                LCD_BG_LAYER_ADDRESS[base] = argb;
                LCD_BG_LAYER_ADDRESS[base + LCD_SCREEN_WIDTH + 1] = argb;
            }
            else {
                LCD_BG_LAYER_ADDRESS[base + 1] = argb;
                LCD_BG_LAYER_ADDRESS[base + LCD_SCREEN_WIDTH] = argb;
            }
#else
            LCD_BG_LAYER_ADDRESS[base] = argb;
            LCD_BG_LAYER_ADDRESS[base + LCD_SCREEN_WIDTH + 1] = argb;
            LCD_BG_LAYER_ADDRESS[base + 1] = argb;
            LCD_BG_LAYER_ADDRESS[base + LCD_SCREEN_WIDTH] = argb;
#endif
        }
    // LCD_BusResume(); LCD_BusPause();
}



void VID_SetPalette(qPal_p palette) { // TODO: make copy and upscale with DMA2D
    // 8 8 8 encoding
    uint32_p table = d_8to24table;
    for (int i = 0; i < InksNum; i++) {
        Rgb32_t r = palette->ink[i].r;
        Rgb32_t g = palette->ink[i].g;
        Rgb32_t b = palette->ink[i].b;

        // uint32_t v = (0xFF << 24) | (r << 16) | (g << 8) | (b << 0);
        // uint32_t v = (b << 24) | (g << 16) | (r << 8) | (0xFF << 0);
        Rgb32_t v = (0xFF << 24) | (b << 16) | (g << 8) | (r << 0);
        *table++ = v;
    }
    d_8to24table[InksNum - 1] &= 0x00FFFFFF; // 255 is transparent
}

void VID_ShiftPalette(qPal_p p) {
    VID_SetPalette(p);
}

void VID_Shutdown() { printf(RED("VID_Shutdown\n")); }