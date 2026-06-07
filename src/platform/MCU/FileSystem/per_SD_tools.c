#include <stdio.h>
#include "terminal_tools.h"
#include "perepherial.h"

#include "SD_TF.h"
#include "MBR.h"
#include "fs_FAT32.h"


/*=======================[SysFS part end]=======================*/
SD_HandleTypeDef hsd2 = {
    .Instance   = SDMMC2,
    .Init       = {
        .ClockEdge              = SDMMC_CLOCK_EDGE_RISING,
        .ClockBypass            = SDMMC_CLOCK_BYPASS_DISABLE,
        .ClockPowerSave         = SDMMC_CLOCK_POWER_SAVE_DISABLE,
        .BusWide                = SDMMC_BUS_WIDE_1B,
        .HardwareFlowControl    = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE,
        .ClockDiv               = 7,
    }
};

extern HAL_SD_CardInfoTypeDef sd_info;

uint8_t SD_InitAndGetInfo() {
    if (HAL_SD_Init(&hsd2) != HAL_OK)   return 1;   // init error

    // TODO: fix for 4 wire mode
#if 0
    if (HAL_SD_ConfigWideBusOperation(&hsd2, SDMMC_BUS_WIDE_4B) != HAL_OK)  return 2;   // bus config error
#endif

    if (HAL_SD_GetCardInfo(&hsd2, &sd_info) != HAL_OK)  return 3;   // card info error
    // SD_PrintCardInfo(&sd_info);

    return 0;
}

static bool s_sdfs_inited = false;

int SD_FS_Init() {
    if (s_sdfs_inited) return 0;
    if (SD_InitAndGetInfo() != 0) { printf(RED("SD_FS_Init: SD init failed\n")); return -1; }
    if (FAT32_Mount(&g_fat32) != 0) { printf(RED("SD_FS_Init: FAT32 mount failed\n")); return -1; }

    s_sdfs_inited = true;
    return 0;
}

void MX_SDMMC2_SD_Init() {
    if (SD_InitAndGetInfo() == 0) {
        SD_WaitCardReady();
        if (SD_MBR_Init())  printf("SD " GREEN("inited\n"));
        else                printf("SD " RED("NOT inited\n"));
        // SD_PrintMBR();

    }
    else {
        printf("SD ERROR\n");
    }
}