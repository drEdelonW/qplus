#pragma once
#include "types.h"

extern uint32_t g_part1_lba_start;
void SD_PrintMBR();
void SD_MBR_Print();
int SD_MBR_Init();