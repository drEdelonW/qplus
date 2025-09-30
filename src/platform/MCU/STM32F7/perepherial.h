#pragma once
#include "main.h"
// #include "stm32f7xx_hal.h"

// extern ETH_TxPacketConfig TxConfig;
// extern ADC_HandleTypeDef hadc1;
// extern ADC_HandleTypeDef hadc3;
// extern CRC_HandleTypeDef hcrc;
// extern DMA2D_HandleTypeDef hdma2d;
// extern DSI_HandleTypeDef hdsi;
// extern ETH_HandleTypeDef heth;
// extern CEC_HandleTypeDef hcec;
// extern I2C_HandleTypeDef hi2c1;
// extern I2C_HandleTypeDef hi2c4;
// extern IWDG_HandleTypeDef hiwdg;
extern LTDC_HandleTypeDef hltdc;
// extern QSPI_HandleTypeDef hqspi;
// extern RTC_HandleTypeDef hrtc;
// extern SAI_HandleTypeDef hsai_BlockA1;
// extern SAI_HandleTypeDef hsai_BlockB1;
// extern SAI_HandleTypeDef hsai_BlockA2;
// extern SD_HandleTypeDef hsd2;
// extern SPDIFRX_HandleTypeDef hspdif;
// extern SPI_HandleTypeDef hspi2;
// extern TIM_HandleTypeDef htim1;
// extern TIM_HandleTypeDef htim3;
// extern TIM_HandleTypeDef htim10;
// extern TIM_HandleTypeDef htim11;
// extern TIM_HandleTypeDef htim12;
// extern UART_HandleTypeDef huart5;
// extern UART_HandleTypeDef huart1;
// extern UART_HandleTypeDef huart6;
// extern PCD_HandleTypeDef hpcd_USB_OTG_HS;
// extern WWDG_HandleTypeDef hwwdg;
// extern SDRAM_HandleTypeDef hsdram1;

void CoreClock_Init();
void Pereph_Init();
