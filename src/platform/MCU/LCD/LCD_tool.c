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
void LCD_BusPause(void) {
    HAL_DSI_Stop(&hdsi_discovery);
    __HAL_LTDC_DISABLE(&hltdc_discovery);
}

/* Resume LCD streaming after heavy SD / FS operations */
void LCD_BusResume(void) {
    __HAL_LTDC_ENABLE(&hltdc_discovery);
    HAL_DSI_Start(&hdsi_discovery);
}
void LCD_Init() {
    printf(TEXT_RED"LCD_Init\n"TEXT_RESET);

    /*##-1- Configure LCD ######################################################*/
       /* LCD DSI initialization in mode Video Burst */
    /* Initialize DSI LCD */
    BSP_LCD_Init();
    BSP_LCD_Clear(Color_Black); BSP_LCD_Clear(Color_Black); BSP_LCD_Clear(Color_Black);

    // BSP_LCD_LayerDefaultInit(background, LCD_FB_START_ADDRESS);
    // BSP_LCD_SetTransparency(background, 0x80);
    // BSP_LCD_SelectLayer(background);    /* Select the LCD Background Layer */
    // BSP_LCD_Clear(Color_Blue);          /* Clear the Background Layer */

    BSP_LCD_LayerDefaultInit(foreground, LCD_BG_LAYER_ADDRESS);
    BSP_LCD_SetTransparency(foreground, 0xFF);
    BSP_LCD_SelectLayer(foreground);    /* Select the LCD Foreground Layer */
    BSP_LCD_Clear(Color_DarkDarkGray);           /* Clear the Foreground Layer */

    /* Configure the transparency for foreground and background : Increase the transparency */
    /*##-1- Configure LCD END ######################################################*/


    // BSP_LCD_SetBrightness(0xFF);
    BSP_LCD_SetTextColor(Color_White);
    BSP_LCD_SetBackColor(Color_Black);
    // BSP_LCD_SetFont(&Font24);
    // BSP_LCD_DisplayStringAtLine(0, (uint8_t*)"!!!LCD WORK!!!");
    BSP_LCD_DisplayStringAt(0, 0, "!!!LCD WORK!!!", CENTER_MODE);
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_DisplayStringAtLine(4, "!!!LCD WORK!!!");
    BSP_LCD_DisplayStringAt(0, 440, "!!!LCD WORK!!!", CENTER_MODE);
    int y = 480;
#if 1
    for (int i = 0; i < y; i++) {
        BSP_LCD_DrawPixel(i, i, Color_Black);
        BSP_LCD_DrawPixel(i + y, y - i, Color_Black);
    }
#else
    BSP_LCD_SetTextColor(Color_Black);
    BSP_LCD_DrawLine(0, 0, y, y);
    BSP_LCD_DrawLine(y + y, 0, y, y);
#endif
    // LCD_BusPause();
    // while (1) { ; }

}

void VID_Update(vRect_p rects) {
    for (int y = 0; y < vid.height; y++)
        for (int x = 0; x < vid.width; x++) {
            uint8_t idx = vid.buffer[y * vid.width + x];

            uint8_t r = d_8to24table[idx] >> 0;
            uint8_t g = d_8to24table[idx] >> 8;
            uint8_t b = d_8to24table[idx] >> 16;

            uint32_t argb =
                Color_Transparent |   // alpha
                (r << 16) |
                (g << 8) |
                (b << 0);
#if 0
            LCD_BG_LAYER_ADDRESS[(x + y * LCD_SCREEN_WIDTH)] = argb;
#else
            int base = (x * 2) + (y * 2) * LCD_SCREEN_WIDTH;

            LCD_BG_LAYER_ADDRESS[base] = argb;
            LCD_BG_LAYER_ADDRESS[base + 1] = argb;
            LCD_BG_LAYER_ADDRESS[base + LCD_SCREEN_WIDTH] = argb;
            LCD_BG_LAYER_ADDRESS[base + LCD_SCREEN_WIDTH + 1] = argb;
#endif
        }
}



void VID_SetPalette(uint8_p palette) { // TODO: make copy and upscale with DMA2D
    printf(TEXT_RED "VID_SetPalette\n" TEXT_RESET);
    // 8 8 8 encoding
    uint8_p pal = palette;
    uint32_p table = d_8to24table;
    for (int i = 0; i < 256; i++) {
        uint32_t r = pal[0];
        uint32_t g = pal[1];
        uint32_t b = pal[2];
        pal += 3;

        // uint32_t v = (255 << 24) + (r << 16) + (g << 8) + (b << 0);
        // uint32_t v = (255 << 0) + (r << 8) + (g << 16) + (b << 24);
        uint32_t v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
        *table++ = v;
    }
    d_8to24table[255] &= 0xffffff;	// 255 is transparent

}

void VID_ShiftPalette(uint8_p p) {
    VID_SetPalette(p);
}
// void D_BeginDirectRect(int x, int y, uint8_p pbitmap, int width, int height) { printf(TEXT_RED "D_BeginDirectRect\n" TEXT_RESET); }
// void D_EndDirectRect(int x, int y, int width, int height) { printf(TEXT_RED "D_EndDirectRect\n" TEXT_RESET); }

void VID_Shutdown() { printf(TEXT_RED "VID_Shutdown\n" TEXT_RESET); }