#pragma once
/**
  ******************************************************************************
  * @file    stm32f769i_discovery_lcd.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32469i_discovery_lcd.c driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
/* Include LCD component Driver */

#include "types.h"
// #include "../Components/otm8009a/otm8009a.h"    /* Include OTM8009A LCD Driver IC driver code */
#include "../Components/nt35510/nt35510.h"      /* Include NT35510 LCD Driver IC driver code */
// #include "../Components/adv7533/adv7533.h"   /* Include ADV7533 HDMI Driver IC driver code */
#include "../../../Utilities/Fonts/fonts.h"

#include "stm32f769i_discovery_sdram.h"         /* Include SDRAM Driver */
#include "stm32f769i_discovery.h"

#include <string.h> /* use of memset() */

/** @addtogroup BSP */
/** @addtogroup STM32F769I_DISCOVERY */
/** @addtogroup STM32F769I_DISCOVERY_LCD  */
/** @defgroup STM32F769I_DISCOVERY_LCD_Exported_Constants STM32F769I DISCOVERY LCD Exported Constants */
#define BSP_LCD_DMA2D_IRQHandler        DMA2D_IRQHandler
#define BSP_LCD_DSI_IRQHandler          DSI_IRQHandler
#define BSP_LCD_LTDC_IRQHandler         LTDC_IRQHandler
#define BSP_LCD_LTDC_ER_IRQHandler      LTDC_ER_IRQHandler

#define LCD_LayerCfgTypeDef             LTDC_LayerCfgTypeDef
#define LCD_FB_START_ADDRESS            ((uint32_t)0xC0000000)  /** LCD FB_StartAddress  */

/** Possible values of pixel data format (ie color coding) transmitted on DSI Data lane in DSI packets */
typedef enum {
    rgb888 = DSI_RGB888, /* DSI packet pixel format chosen is RGB888 : 24 bpp */
    rgb565 = DSI_RGB565  /* DSI packet pixel format chosen is RGB565 : 16 bpp */
} LcdPixelFmt_t;

typedef enum {
    background = 0u, /* LTDC Background layer index */
    foreground = 1u, /* LTDC Foreground layer index */
    layers_max = 2u  /* Maximum number of LTDC layers */
} LtdcLayer_t;

typedef enum {
    Ok      = 0u, /* LCD OK status */
    Error   = 1u, /* LCD error status */
    timeout = 2u  /* LCD timeout status */
} LcdStatus_t;

#define LCD_Driver_ID  ((uint32_t) 0)   /**  LCD Display DSI Virtual Channel  ID    */

/**  HDMI Foramt    */
#define HDMI_FORMAT_720_480   ((uint8_t) 0x00) /*720_480 format choice of HDMI display */
#define HDMI_FORMAT_720_576   ((uint8_t) 0x01) /*720_576 format choice of HDMI display*/

typedef uint32_t Color_t;
/** LCD color definitions values in ARGB8888 format. */
typedef enum lcd_color_argb8888_e { /* value in ARGB8888 format */
    Color_Transparent  = 0xFF000000u, /* Transparent value in ARGB8888 format (as in original defines) */
    Color_White        = 0xFFFFFFFFu, /* White */
    Color_Black        = 0xFF000000u, /* Black */
    Color_Blue         = 0xFF0000FFu, /* Blue */
    Color_Green        = 0xFF00FF00u, /* Green */
    Color_Red          = 0xFFFF0000u, /* Red */
    Color_Cyan         = 0xFF00FFFFu, /* Cyan */
    Color_Magenta      = 0xFFFF00FFu, /* Magenta */
    Color_Yellow       = 0xFFFFFF00u, /* Yellow */
    Color_Orange       = 0xFFFFA500u, /* Orange */

    Color_Gray         = 0xFF808080u, /* Gray */
    Color_LightBlue    = 0xFF8080FFu, /* Light Blue */
    Color_LightGreen   = 0xFF80FF80u, /* Light Green */
    Color_LightRed     = 0xFFFF8080u, /* Light Red */
    Color_LightCyan    = 0xFF80FFFFu, /* Light Cyan */
    Color_LightMagenta = 0xFFFF80FFu, /* Light Magenta */
    Color_LightYellow  = 0xFFFFFF80u, /* Light Yellow */
    Color_LightGray    = 0xFFD3D3D3u, /* Light Gray */

    Color_Brown        = 0xFFA52A2Au, /* Brown */
    Color_DarkGray     = 0xFF404040u, /* Dark Gray */
    Color_DarkBlue     = 0xFF000080u, /* Dark Blue */
    Color_DarkGreen    = 0xFF008000u, /* Dark Green */
    Color_DarkRed      = 0xFF800000u, /* Dark Red */
    Color_DarkCyan     = 0xFF008080u, /* Dark Cyan */
    Color_DarkMagenta  = 0xFF800080u, /* Dark Magenta */
    Color_DarkYellow   = 0xFF808000u, /* Dark Yellow */
} lcd_color_argb8888_t;

/** LCD default font */
#define LCD_DEFAULT_FONT        Font24
// #define LCD_DEFAULT_FONT        Font8


/** @defgroup STM32F769I_DISCOVERY_LCD_Exported_Types STM32F769I DISCOVERY LCD Exported Types */
/** LCD Drawing main properties */
typedef struct {
    Color_t TextColor; /*!< Specifies the color of text */
    Color_t BackColor; /*!< Specifies the background color below the text */
    sFONT* pFont;    /*!< Specifies the font used for the text */
} LCD_DrawPropTypeDef;

typedef struct {    /** LCD Drawing point (pixel) geometric definition */
    int16_t X;      /*!< geometric X position of drawing */
    int16_t Y;      /*!< geometric Y position of drawing */
} Point;
typedef Point* pPoint;  /** Pointer on LCD Drawing point (pixel) geometric definition */

typedef enum {      /** LCD drawing Line alignment mode definitions */
    CENTER_MODE,    /*!< Center mode */
    RIGHT_MODE,     /*!< Right mode  */
    LEFT_MODE       /*!< Left mode   */
} Text_AlignModeTypdef;


/** LCD_Orientation_t. Possible values of Display Orientation */
typedef enum {
    LCD_ORIENTATION_PORTRAIT    = 0x00, /*!< Portrait orientation choice of LCD screen  */
    LCD_ORIENTATION_LANDSCAPE   = 0x01, /*!< Landscape orientation choice of LCD screen */
    LCD_ORIENTATION_INVALID     = 0x02  /*!< Invalid orientation choice of LCD screen   */
} LCD_Orientation_t;


#ifdef __cplusplus
extern "C" {
#endif

    /** @defgroup STM32F769I_DISCOVERY_LCD_Exported_Macro STM32F769I DISCOVERY LCD Exported Macro */
    /** @addtogroup STM32F769I_DISCOVERY_LCD_Exported_Functions */
    uint8_t  BSP_LCD_Init(void);
    uint8_t  BSP_LCD_InitEx(LCD_Orientation_t orientation);
    uint8_t  BSP_LCD_HDMIInitEx(uint8_t format);

    void     BSP_LCD_MspDeInit(void);
    void     BSP_LCD_MspInit(void);
    void     BSP_LCD_Reset(void);

    uint32_t BSP_LCD_GetXSize(void);
    uint32_t BSP_LCD_GetYSize(void);
    void     BSP_LCD_SetXSize(uint32_t imageWidthPixels);
    void     BSP_LCD_SetYSize(uint32_t imageHeightPixels);

    void     BSP_LCD_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address);
    void     BSP_LCD_SetTransparency(LtdcLayer_t LayerIndex, uint8_t Transparency);
    void     BSP_LCD_SetLayerAddress(LtdcLayer_t LayerIndex, uint32_t Address);
    void     BSP_LCD_SetColorKeying(LtdcLayer_t LayerIndex, uint32_t RGBValue);
    void     BSP_LCD_ResetColorKeying(LtdcLayer_t LayerIndex);
    void     BSP_LCD_SetLayerWindow(uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);

    void     BSP_LCD_SelectLayer(LtdcLayer_t LayerIndex);
    void     BSP_LCD_SetLayerVisible(LtdcLayer_t LayerIndex, FunctionalState State);

    void     BSP_LCD_SetTextColor(Color_t Color);
    Color_t  BSP_LCD_GetTextColor(void);
    void     BSP_LCD_SetBackColor(Color_t Color);
    Color_t  BSP_LCD_GetBackColor(void);
    void     BSP_LCD_SetFont(sFONT* fonts);
    sFONT*   BSP_LCD_GetFont(void);

    Color_t  BSP_LCD_ReadPixel(uint16_t Xpos, uint16_t Ypos);
    void     BSP_LCD_DrawPixel(uint16_t Xpos, uint16_t Ypos, Color_t pixel);

    void     BSP_LCD_Clear(Color_t Color);
    void     BSP_LCD_ClearStringLine(uint32_t Line);
    void     BSP_LCD_DisplayStringAtLine(uint16_t Line, cStringRO ptr);
    void     BSP_LCD_DisplayStringAt(uint16_t Xpos, uint16_t Ypos, cStringRO Text, Text_AlignModeTypdef Mode);
    void     BSP_LCD_DisplayChar(uint16_t Xpos, uint16_t Ypos, uint8_t Ascii);

    void     BSP_LCD_DrawHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length);
    void     BSP_LCD_DrawVLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length);
    void     BSP_LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

    void     BSP_LCD_DrawRect(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
    void     BSP_LCD_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);

    void     BSP_LCD_DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius);
    void     BSP_LCD_FillCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius);

    void     BSP_LCD_DrawPolygon(pPoint Points, uint16_t PointCount);
    void     BSP_LCD_FillPolygon(pPoint Points, uint16_t PointCount);

    void     BSP_LCD_DrawEllipse(int Xpos, int Ypos, int XRadius, int YRadius);
    void     BSP_LCD_FillEllipse(int Xpos, int Ypos, int XRadius, int YRadius);

    void     BSP_LCD_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t* pbmp);

    void     BSP_LCD_DisplayOn(void);
    void     BSP_LCD_DisplayOff(void);
    void     BSP_LCD_SetBrightness(uint8_t BrightnessValue);

    /** @defgroup STM32F769I_DISCOVERY_LCD_Exported_Variables STM32F769I DISCOVERY LCD Exported Variables */
    /* @brief DMA2D handle variable */
    extern DMA2D_HandleTypeDef hdma2d_discovery;

#ifdef __cplusplus
}
#endif


