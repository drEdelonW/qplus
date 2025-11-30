/**
  ******************************************************************************
  * @file    Display/LCD_PicturesFromSDCard/Src/main.c
  * @author  MCD Application Team
  * @brief   This file provides main program functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

  /* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
/** @addtogroup STM32F7xx_HAL_Applications */
/** @addtogroup LCD_PicturesFromSDCard */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
// FATFS SD_FatFs;  /* File system object for SD card logical drive */
// char SD_Path[4]; /* SD card logical drive path */
// #define MAX_BMP_FILES     25
// #define MAX_BMP_FILE_NAME 11
// char* pDirectoryFiles[MAX_BMP_FILES];
// uint8_t  ubNumberOfFiles = 0;
// uint32_t uwBmplen = 0;

/* Internal Buffer defined in SDRAM memory */
uint8_t* uwInternalBuffer;

/* Private function prototypes -----------------------------------------------*/
static void MPU_Config(void);
static void LCD_Config(void);
static void SystemClock_Config(void);
void Error_Handler(void);
static void CPU_CACHE_Enable(void);;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void) {
    // uint32_t counter = 0;
    // uint32_t transparency = 0;
    // uint8_t str[30];
    uwInternalBuffer = (uint8_t*)INTERNAL_BUFFER_START_ADDRESS;

    /* Configure the MPU attributes */
    MPU_Config();

    /* Enable the CPU Cache */
    CPU_CACHE_Enable();

    /* STM32F7xx HAL library initialization:
         - Configure the Flash ART accelerator on ITCM interface
         - Configure the Systick to generate an interrupt each 1 msec
         - Set NVIC Group Priority to 4
         - Global MSP (MCU Support Package) initialization
       */
    HAL_Init();

    /* Configure the system clock to 180 MHz */
    SystemClock_Config();
    MX_USART1_UART_Init();
    /* Configure LED1 */
    // BSP_LED_Init(LED1);

    /*##-1- Configure LCD ######################################################*/
    LCD_Config();

    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetBackColor(LCD_COLOR_GRAY);
    BSP_LCD_DisplayStringAtLine(0, (uint8_t*)"!!!LCD WORK!!!");
    while (1) { ; }
#if 0
    /*##-2- Link the SD Card disk I/O driver ###################################*/
    // if(FATFS_LinkDriver(&SD_Driver, SD_Path) == 0)
    {
        /*##-3- Initialize the Directory Files pointers (heap) ###################*/
        for (counter = 0; counter < MAX_BMP_FILES; counter++) {
            pDirectoryFiles[counter] = malloc(MAX_BMP_FILE_NAME);
            if (pDirectoryFiles[counter] == NULL) {
                /* Set the Text Color */
                BSP_LCD_SetTextColor(LCD_COLOR_RED);

                BSP_LCD_DisplayStringAtLine(8, (uint8_t*)"  Cannot allocate memory ");
                while (1) { ; }
            }
        }

        /* Get the BMP file names on root directory */
        // ubNumberOfFiles = Storage_GetDirectoryBitmapFiles("/Media", pDirectoryFiles);

        if (ubNumberOfFiles == 0) {
            for (counter = 0; counter < MAX_BMP_FILES; counter++) {
                free(pDirectoryFiles[counter]);
            }
            BSP_LCD_DisplayStringAtLine(8, (uint8_t*)"  No more Bitmap files...      ");
            while (1) { ; }
        }
    }
    // else
    // {
    //   /* FatFs Initialization Error */
    //   Error_Handler();
    // }

    /* Main infinite loop */
    while (1) {
        counter = 0;

        while ((counter) < ubNumberOfFiles) {
            /* Step1 : Display on Foreground layer -------------------------------*/
            /* Format the string */
            sprintf((char*)str, "Media/%-11.11s", pDirectoryFiles[counter]);

            // if (Storage_CheckBitmapFile((const char*)str, &uwBmplen) == 0)
            {
                /* Format the string */
                sprintf((char*)str, "Media/%-11.11s", pDirectoryFiles[counter]);

                /* Set LCD foreground Layer */
                BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER_FOREGROUND);

                /* Open a file and copy its content to an internal buffer */
                // Storage_OpenReadFile(uwInternalBuffer, (const char*)str);

                /* Write bmp file on LCD frame buffer */
                BSP_LCD_DrawBitmap(0, 0, uwInternalBuffer);

                /* Configure the transparency for background layer : Increase the transparency */
                for (transparency = 0; transparency < 255; (transparency++)) {
                    BSP_LCD_SetTransparency(LTDC_ACTIVE_LAYER_FOREGROUND, transparency);

                    /* Insert a delay of display */
                    HAL_Delay(10);
                }

                /* Configure the transparency for foreground layer : decrease the transparency */
                for (transparency = 255; transparency > 0; transparency--) {
                    BSP_LCD_SetTransparency(LTDC_ACTIVE_LAYER_FOREGROUND, transparency);

                    /* Insert a delay of display */
                    HAL_Delay(10);
                }

                HAL_Delay(1000);

                /* Clear the Foreground Layer */
                BSP_LCD_Clear(LCD_COLOR_BLACK);

                /* Jump to the next image */
                counter++;

                /* Step2 : Display on Background layer -----------------------------*/
                /* Format the string */
                sprintf((char*)str, "Media/%-11.11s", pDirectoryFiles[counter]);

                // if ((Storage_CheckBitmapFile((const char*)str, &uwBmplen) == 0) || (counter < (ubNumberOfFiles)))
                {
                    /* Connect the Output Buffer to LCD Background Layer  */
                    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER_BACKGROUND);

                    /* Format the string */
                    sprintf((char*)str, "Media/%-11.11s", pDirectoryFiles[counter]);

                    /* Open a file and copy its content to an internal buffer */
                    // Storage_OpenReadFile(uwInternalBuffer, (const char*)str);

                    /* Write bmp file on LCD frame buffer */
                    BSP_LCD_DrawBitmap(0, 0, uwInternalBuffer);

                    /* Configure the transparency for background layer : decrease the transparency */
                    for (transparency = 0; transparency < 255; (transparency++)) {
                        BSP_LCD_SetTransparency(LTDC_ACTIVE_LAYER_BACKGROUND, transparency);

                        /* Insert a delay of display */
                        HAL_Delay(10);
                    }

                    HAL_Delay(1000);

                    /* Step3 : -------------------------------------------------------*/
                    /* Configure the transparency for background layer : Increase the transparency */
                    for (transparency = 255; transparency > 0; transparency--) {
                        BSP_LCD_SetTransparency(LTDC_ACTIVE_LAYER_BACKGROUND, transparency);

                        /* Insert a delay of display */
                        HAL_Delay(10);
                    }

                    HAL_Delay(1000);

                    /* Clear the Background Layer */
                    BSP_LCD_Clear(LCD_COLOR_BLACK);

                    counter++;
                }
                // else if (Storage_CheckBitmapFile((const char*)str, &uwBmplen) == 0)
                {
                    /* Set the Text Color */
                    BSP_LCD_SetTextColor(LCD_COLOR_RED);

                    BSP_LCD_DisplayStringAtLine(7, (uint8_t*)str);
                    BSP_LCD_DisplayStringAtLine(8, (uint8_t*)"    File type not supported. ");
                    while (1) { ; }
                }
            }
        }
    }
#endif
}

/**
  * @brief  LCD configuration
  * @param  None
  * @retval None
  */
static void LCD_Config(void) {
    uint8_t lcd_status = LCD_OK;

    /* LCD DSI initialization in mode Video Burst */
    /* Initialize DSI LCD */
    BSP_LCD_Init();
    while (lcd_status != LCD_OK);

    BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER_BACKGROUND, LCD_FB_START_ADDRESS);

    /* Select the LCD Background Layer */
    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER_BACKGROUND);

    /* Clear the Background Layer */
    BSP_LCD_Clear(LCD_COLOR_WHITE);
    // BSP_LCD_Clear(LCD_COLOR_BLACK);

    BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER_FOREGROUND, LCD_BG_LAYER_ADDRESS);

    /* Select the LCD Foreground Layer */
    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER_FOREGROUND);

    /* Clear the Foreground Layer */
    BSP_LCD_Clear(LCD_COLOR_WHITE);
    // BSP_LCD_Clear(LCD_COLOR_BLACK);

    /* Configure the transparency for foreground and background :
    Increase the transparency */
    BSP_LCD_SetTransparency(LTDC_ACTIVE_LAYER_BACKGROUND, 0);
    BSP_LCD_SetTransparency(LTDC_ACTIVE_LAYER_FOREGROUND, 100);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void) {
    /* Turn LED1 on */
    // BSP_LED_On(LED1);
    while (1) {
    }
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void) {
    /* Enable I-Cache */
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_EnableDCache();
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            PLL_R                          = 6
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void) {
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    RCC_OscInitStruct.PLL.PLLR = 6;

    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if (ret != HAL_OK) {
        while (1) { ; }
    }

    /* Activate the OverDrive to reach the 180 MHz Frequency */
    ret = HAL_PWREx_EnableOverDrive();
    if (ret != HAL_OK) {
        while (1) { ; }
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
    if (ret != HAL_OK) {
        while (1) { ; }
    }
}


/**
  * @brief  Configure the MPU attributes
  * @param  None
  * @retval None
  */
static void MPU_Config(void) {

    /* Disable the MPU */
    HAL_MPU_Disable();

    /* Configure the MPU as Strongly ordered for not defined regions */
    MPU_Region_InitTypeDef MPU_WholeRange = (MPU_Region_InitTypeDef){
        .Enable             = MPU_REGION_ENABLE,
        .Number             = MPU_REGION_NUMBER0,
        .BaseAddress        = 0x00,
        .Size               = MPU_REGION_SIZE_4GB,
        .SubRegionDisable   = 0x87,
        .TypeExtField       = MPU_TEX_LEVEL0,
        .AccessPermission   = MPU_REGION_NO_ACCESS,
        .DisableExec        = MPU_INSTRUCTION_ACCESS_DISABLE,
        .IsShareable        = MPU_ACCESS_SHAREABLE,
        .IsCacheable        = MPU_ACCESS_NOT_CACHEABLE,
        .IsBufferable       = MPU_ACCESS_NOT_BUFFERABLE,
    };
    HAL_MPU_ConfigRegion(&MPU_WholeRange);

    /* Configure the MPU attributes as WT for SDRAM */
    MPU_Region_InitTypeDef MPU_SdramRange = (MPU_Region_InitTypeDef){
        .Enable             = MPU_REGION_ENABLE,
        .Number             = MPU_REGION_NUMBER1,
        .BaseAddress        = 0xC0000000,
        .Size               = MPU_REGION_SIZE_32MB,
        .SubRegionDisable   = 0x00,
        .TypeExtField       = MPU_TEX_LEVEL0,
        .AccessPermission   = MPU_REGION_FULL_ACCESS,
        .DisableExec        = MPU_INSTRUCTION_ACCESS_ENABLE,
        .IsShareable        = MPU_ACCESS_NOT_SHAREABLE,
        .IsCacheable        = MPU_ACCESS_CACHEABLE,
        .IsBufferable       = MPU_ACCESS_NOT_BUFFERABLE
    };

    HAL_MPU_ConfigRegion(&MPU_SdramRange);

    /* Configure the MPU attributes FMC control registers */
    MPU_Region_InitTypeDef MPU_FMCRange = (MPU_Region_InitTypeDef){
        .Enable           = MPU_REGION_ENABLE,
        .Number           = MPU_REGION_NUMBER2,
        .BaseAddress      = 0xA0000000,
        .Size             = MPU_REGION_SIZE_8KB,
        .SubRegionDisable = 0x0,
        .TypeExtField     = MPU_TEX_LEVEL0,
        .AccessPermission = MPU_REGION_FULL_ACCESS,
        .DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE,
        .IsShareable      = MPU_ACCESS_SHAREABLE,
        .IsCacheable      = MPU_ACCESS_NOT_CACHEABLE,
        .IsBufferable     = MPU_ACCESS_BUFFERABLE
    };

    HAL_MPU_ConfigRegion(&MPU_FMCRange);

    /* Enable the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line) {
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

       /* Infinite loop */
    while (1) {
    }
}
#endif

#include "types.h"
#include "perepherial.h"
#include <sys/errno.h>
#define VCP_RX_Pin GPIO_PIN_10
#define VCP_RX_GPIO_Port GPIOA
#define VCP_TX_Pin GPIO_PIN_9
#define VCP_TX_GPIO_Port GPIOA
static void USART1_GPIO_Clock_Init(void) {
    /* Enable clocks for GPIOA and USART1 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* Configure PA9 (VCP_TX) and PA10 (VCP_RX) as USART1 AF7 */
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = VCP_TX_Pin | VCP_RX_Pin;   // PA9, PA10
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

UART_HandleTypeDef huart1 = {
    .Instance = USART1,
    .Init = {
        .BaudRate = 115200,
        .WordLength = UART_WORDLENGTH_8B,
        .StopBits = UART_STOPBITS_1,
        .Parity = UART_PARITY_NONE,
        .Mode = UART_MODE_TX_RX,
        .HwFlowCtl = UART_HWCONTROL_NONE,
        .OverSampling = UART_OVERSAMPLING_16,
        .OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE,
    },
    .AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT,
};

void MX_USART1_UART_Init() {
    USART1_GPIO_Clock_Init();

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

    /* Raw init message over UART1 */
    {
        const char msg[] = "UART1 init OK\r\n";
        (void)HAL_UART_Transmit(
            &huart1,
            (uint8_p)msg,
            (uint16_t)(sizeof(msg) - 1),
            HAL_MAX_DELAY
        );
    }
}

int _write(int file, const char *buf, int len) {
    // stdout(1) и stderr(2) отправляем в UART
    if ((file == 1) || (file == 2)) {
        HAL_StatusTypeDef st = HAL_UART_Transmit(
            &huart1,
            (uint8_p)buf,
            (uint16_t)len,
            HAL_MAX_DELAY
        );

        if (st == HAL_OK) {
            return len;
        } else {
            errno = EIO;
            return -1;
        }
    }

    // остальные файловые дескрипторы не поддерживаем
    errno = EBADF;
    return -1;
}