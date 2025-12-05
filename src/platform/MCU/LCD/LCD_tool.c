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
uint8_t LCD_FG_LAYER_ADDRESS[LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT * ARGB8888_BYTE_PER_PIXEL] PLACE_TO_SDRAM;
// uint8_t LCD_BG_LAYER_ADDRESS[LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT * ARGB8888_BYTE_PER_PIXEL] PLACE_TO_SDRAM;
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

    BSP_LCD_LayerDefaultInit(background, LCD_FB_START_ADDRESS);
    BSP_LCD_SetTransparency(background, 0x80);
    BSP_LCD_SelectLayer(background);    /* Select the LCD Background Layer */
    BSP_LCD_Clear(Color_Blue);          /* Clear the Background Layer */

    // BSP_LCD_LayerDefaultInit(foreground, LCD_BG_LAYER_ADDRESS);
    // BSP_LCD_SetTransparency(foreground, 0x80);
    // BSP_LCD_SelectLayer(foreground);    /* Select the LCD Foreground Layer */
    // BSP_LCD_Clear(Color_Red);           /* Clear the Foreground Layer */

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
    vid.buffer = LCD_FG_LAYER_ADDRESS;
}