/**
  ******************************************************************************
  * @file    Display/LCD_PicturesFromSDCard/Src/stm32f7xx_it.c
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
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
#include "stm32f7xx_it.h"

/** @addtogroup STM32F7xx_HAL_Applications */
/** @addtogroup LCD_PicturesFromSDCard */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern SD_HandleTypeDef uSdHandle;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M7 Processor Exceptions Handlers                         */
/******************************************************************************/

void NMI_Handler(void) {} /** This function handles NMI exception. */

/** This function handles Hard Fault exception. */
void HardFault_Handler(void) {
  while (1) {}  /* Go to infinite loop when Hard Fault exception occurs */
}

/** This function handles Memory Manage exception. */
void MemManage_Handler(void) {
  while (1) {}  /* Go to infinite loop when Memory Manage exception occurs */
}

/** This function handles Bus Fault exception. */
void BusFault_Handler(void) {
  while (1) {} /* Go to infinite loop when Bus Fault exception occurs */
}

/** This function handles Usage Fault exception. */
void UsageFault_Handler(void) {
  while (1) {} /* Go to infinite loop when Usage Fault exception occurs */
}

void SVC_Handler(void) {}       /** This function handles SVCall exception. */
void DebugMon_Handler(void) {}  /** This function handles Debug Monitor exception. */
void PendSV_Handler(void) {}    /** This function handles PendSVC exception. */

/** This function handles SysTick Handler. */
void SysTick_Handler(void) {
  HAL_IncTick();
}

/** Handles SDMMC2 DMA Rx transfer interrupt request. */
void BSP_SDMMC2_DMA_Rx_IRQHandler(void) {
  HAL_DMA_IRQHandler(uSdHandle.hdmarx);
}

/** Handles SDMMC2 DMA Tx transfer interrupt request. */
void BSP_SDMMC2_DMA_Tx_IRQHandler(void) {
  HAL_DMA_IRQHandler(uSdHandle.hdmatx);
}

/** Handles SD1 card interrupt request. */
void BSP_SDMMC2_IRQHandler(void) {
  HAL_SD_IRQHandler(&uSdHandle);
}

/******************************************************************************/
/*                 STM32F7xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f7xx.s).                                               */
/******************************************************************************/

/** This function handles PPP interrupt request. */
/* void PPP_IRQHandler(void) {} */
