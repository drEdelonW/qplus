#pragma once
#include "types.h"
#include "stm32f7xx_hal.h"

uint16_t rd16_le(cStringRO p);
uint32_t rd32_le(cStringRO p);

void SD_PrintCardInfo(const HAL_SD_CardInfoTypeDef* info);
HAL_StatusTypeDef SD_ReadBlock(uint32_t lba, uint8_p buf);
HAL_StatusTypeDef SD_DumpBlock(uint32_t lba);
void SD_WaitCardReady();