#include <stdio.h>
#include "perepherial.h"
#include "fs_FAT32.h"
#include "SD_TF.h"


SD_HandleTypeDef hsd2 = {
    .Instance = SDMMC2,
    .Init = {
        .ClockEdge              = SDMMC_CLOCK_EDGE_RISING,
        .ClockBypass            = SDMMC_CLOCK_BYPASS_DISABLE,
        .ClockPowerSave         = SDMMC_CLOCK_POWER_SAVE_DISABLE,
        .BusWide                = SDMMC_BUS_WIDE_1B,
        .HardwareFlowControl    = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE,
        .ClockDiv               = 0,
    }
};


extern HAL_SD_CardInfoTypeDef sd_info;

uint8_t SD_InitAndGetInfo(void) {
    if (HAL_SD_Init(&hsd2) != HAL_OK)   return 1;   // init error

#if 0    // TODO: fix for 4 wire mode
    if (HAL_SD_ConfigWideBusOperation(&hsd2, SDMMC_BUS_WIDE_4B) != HAL_OK) {
        return 2;   // bus config error
    }
#endif

    if (HAL_SD_GetCardInfo(&hsd2, &sd_info) != HAL_OK) {
        return 3;   // card info error
    }

    // SD_PrintCardInfo(&sd_info);

    return 0;
}


static void SDFS_MakeFatPath(cStringRO quakePath, char* out, uint32_t outSize) {
    // базовый префикс для карты
    cStringRO base = "QUAKE/";
    uint32_t i = 0;

    // копируем префикс
    while (*base && i + 1 < outSize) {
        out[i++] = *base++;
    }

    // копируем исходный путь, поднимая регистр и заменяя '\' на '/'
    for (cStringRO p = quakePath; *p && i + 1 < outSize; ++p) {
        char c = *p;
        if (c == '\\')  c = '/';
        // uppercase латиницу
        if ((c >= 'a') && (c <= 'z'))   c = (char)(c - 'a' + 'A');
        out[i++] = c;
    }

    out[i] = '\0';
}

#define SDFS_MAX_OPEN_FILES 8

typedef struct {
    bool          used;
    FAT32_File_t  file;
} SD_FileSlot;

static SD_FileSlot s_sdFiles[SDFS_MAX_OPEN_FILES];

static int SDFS_AllocHandle(void) {
    for (int i = 0; i < SDFS_MAX_OPEN_FILES; ++i) {
        if (!s_sdFiles[i].used) {
            s_sdFiles[i].used = true;
            return i;
        }
    }
    return -1;
}

static SD_FileSlot* SDFS_GetSlot(int hnd) {
    if ((hnd < 0) ||
        (hnd >= SDFS_MAX_OPEN_FILES) ||
        (!s_sdFiles[hnd].used))         return NULL;
    return &s_sdFiles[hnd];
}


static bool s_sdfs_inited = false;

int SD_FS_Init() {
    if (s_sdfs_inited) return 0;

    if (SD_InitAndGetInfo() != 0) {
        printf("SD_FS_Init: SD init failed\n");
        return -1;
    }

    if (FAT32_Mount(&g_fat32) != 0) {
        printf("SD_FS_Init: FAT32 mount failed\n");
        return -1;
    }

    s_sdfs_inited = true;
    return 0;
}

int Sys_FileOpenRead(cStringRO quakePath, int* hnd) {
    if (!hnd) return -1;

    if (SD_FS_Init() != 0) {
        *hnd = -1;
        return -1;
    }

    char fatPath[128];
    SDFS_MakeFatPath(quakePath, fatPath, sizeof(fatPath));

    FAT32_File_t f;
    if (FAT32_FileOpen(fatPath, &f) != 0) {
        // можно отладочно писать:
        // printf("Sys_FileOpenRead: '%s' -> '%s' not found\n", quakePath, fatPath);
        *hnd = -1;
        return -1;
    }

    int slot = SDFS_AllocHandle();
    if (slot < 0) {
        FAT32_FileClose(&f);
        *hnd = -1;
        return -1;
    }

    s_sdFiles[slot].file = f;

    *hnd = slot;
    return (int)f.file_size; // по контракту Quake возвращает длину
}

void Sys_FileClose(int handle) {
    SD_FileSlot* slot = SDFS_GetSlot(handle);
    if (!slot) return;

    FAT32_FileClose(&slot->file);
    slot->used = 0;
}

int Sys_FileRead(int handle, void* dest, int count) {
    SD_FileSlot* slot = SDFS_GetSlot(handle);
    if (!slot) return 0;

    uint32_t got = 0;
    if (FAT32_FileRead(&slot->file, dest, (uint32_t)count, &got) != 0) {
        return 0;
    }
    return (int)got;
}

int Sys_FileSeek(int handle, int position) {
    SD_FileSlot* slot = SDFS_GetSlot(handle);
    if (!slot) return -1;

    if (FAT32_FileSeekSet(&slot->file, (uint32_t)position) != 0) {
        return -1;
    }
    return 0;
}



void MX_SDMMC2_SD_Init() {
    if (SD_InitAndGetInfo() == 0) {
        SD_WaitCardReady();
        printf("SD inited\n");
        SD_PrintMBR();

#if 0
        printf("MX_SDMMC2_SD_Init\n");
        if (FAT32_Mount(&g_fat32) == 0) {
            // FAT32_PrintTree();

            // FAT32_File_t f;
            // if (FAT32_FileOpen("QUAKE/ID1/PAK0.PAK", &f) == 0) {
            //     printf("PAK0.PAK size: %lu bytes\n", (uint32_t)f.file_size);

            //     uint8_t header[64];
            //     uint32_t got = 0;
            //     if (FAT32_FileRead(&f, header, sizeof(header), &got) == 0) {
            //         printf("Read first %lu bytes:\n", (uint32_t)got);
            //         for (uint32_t i = 0; i < got; i += 16) {
            //             printf("%04lX: ", (uint32_t)i);
            //             for (uint32_t j = 0; j < 16 && i + j < got; ++j) {
            //                 printf("%02X ", (uint32_t)header[i + j]);
            //             }
            //             printf("\n");
            //         }
            //     }

            //     FAT32_FileClose(&f);
            // }
        }
        else {
            printf("MX_SDMMC2_SD_Init error\n");
        }
#endif
    }
    else {
        printf("SD ERROR\n");
    }
}