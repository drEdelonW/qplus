#include "SD_TF.h"
#include <stdio.h>

#define SD_READ_TIMEOUT_MS  100

uint16_t rd16_le(cStringRO p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t rd32_le(cStringRO p) {
    return  (uint32_t)p[0]
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);
}



static cStringRO SD_GetCardTypeStr(uint32_t t) {
#ifdef CARD_SDHC_SDXC
    if (t == CARD_SDHC_SDXC)    return "SDHC / SDXC";
#endif
#ifdef CARD_SDSC
    if (t == CARD_SDSC)         return "SDSC";
#endif
#ifdef CARD_SECURED
    if (t == CARD_SECURED)      return "Secured";
#endif
#ifdef CARD_MMC
    if (t == CARD_MMC)          return "MMC";
#endif
    return "Unknown";
}

static cStringRO SD_GetCardVersionStr(uint32_t v) {
#ifdef CARD_V1_X
    if (v == CARD_V1_X)     return "SD v1.x";
#endif
#ifdef CARD_V2_X
    if (v == CARD_V2_X)     return "SD v2.x / SDHC / SDXC";
#endif
    return "Unknown";
}


void SD_PrintCardInfo(const HAL_SD_CardInfoTypeDef* info) {
    if (!info) {
        printf("SD card info: null pointer\n");
        return;
    }

    uint64_t phys_bytes = (uint64_t)info->BlockNbr * (uint64_t)info->BlockSize;
    uint64_t log_bytes = (uint64_t)info->LogBlockNbr * (uint64_t)info->LogBlockSize;

    uint32_t phys_mb = (uint32_t)(phys_bytes / (1024u * 1024u));
    uint32_t log_mb = (uint32_t)(log_bytes / (1024u * 1024u));

    uint64_t last_log_byte =
        (info->LogBlockNbr > 0u)
        ? (log_bytes - 1u)
        : 0u;

    printf("\n=== SD Card Info ======================\n");
    printf("CardType      : 0x%08lX (%s)\n", (uint32_t)info->CardType, SD_GetCardTypeStr(info->CardType));
    printf("CardVersion   : 0x%08lX (%s)\n", (uint32_t)info->CardVersion, SD_GetCardVersionStr(info->CardVersion));
    printf("Class         : 0x%08lX\n", (uint32_t)info->Class);
    printf("RCA           : 0x%08lX\n", (uint32_t)info->RelCardAdd);
    printf("\nPhysical capacity:\n");
    printf("  BlockNbr    : %lu blocks\n", (uint32_t)info->BlockNbr);
    printf("  BlockSize   : %lu bytes\n", (uint32_t)info->BlockSize);
    printf("  Total       : %lu MB (approx)\n", (uint32_t)phys_mb);
    printf("\nLogical capacity:\n");
    printf("  LogBlockNbr : %lu blocks\n", (uint32_t)info->LogBlockNbr);
    printf("  LogBlockSize: %lu bytes\n", (uint32_t)info->LogBlockSize);
    printf("  Total       : %lu MB (approx)\n", (uint32_t)log_mb);
    printf("\nAddressing:\n");
    printf("  Last LBA    : %lu\n", (uint32_t)(info->LogBlockNbr ? (info->LogBlockNbr - 1u) : 0u));
    printf("  Last byte   : 0x%08lX%08lX\n", (uint32_t)(last_log_byte >> 32), (uint32_t)(last_log_byte & 0xFFFFFFFFu));
    printf("=======================================\n\n");
}




static void SD_PrintHalSdError(uint32_t err) {
    printf("SD error mask: 0x%08lX\n", (uint32_t)err);

#ifdef HAL_SD_ERROR_NONE
    if (err == HAL_SD_ERROR_NONE) { printf("  HAL_SD_ERROR_NONE\n");        return; }
#endif

#ifdef HAL_SD_ERROR_ADDR_OUT_OF_RANGE
    if (err & HAL_SD_ERROR_ADDR_OUT_OF_RANGE)       printf("  HAL_SD_ERROR_ADDR_OUT_OF_RANGE\n");
#endif
#ifdef HAL_SD_ERROR_ADDR_MISALIGNED
    if (err & HAL_SD_ERROR_ADDR_MISALIGNED)         printf("  HAL_SD_ERROR_ADDR_MISALIGNED\n");
#endif
#ifdef HAL_SD_ERROR_BLOCK_LEN_ERR
    if (err & HAL_SD_ERROR_BLOCK_LEN_ERR)           printf("  HAL_SD_ERROR_BLOCK_LEN_ERR\n");
#endif
#ifdef HAL_SD_ERROR_ERASE_SEQ_ERR
    if (err & HAL_SD_ERROR_ERASE_SEQ_ERR)           printf("  HAL_SD_ERROR_ERASE_SEQ_ERR\n");
#endif
#ifdef HAL_SD_ERROR_BAD_CID
    if (err & HAL_SD_ERROR_BAD_CID)                 printf("  HAL_SD_ERROR_BAD_CID\n");
#endif
#ifdef HAL_SD_ERROR_WRITE_PROT_VIOLATION
    if (err & HAL_SD_ERROR_WRITE_PROT_VIOLATION)    printf("  HAL_SD_ERROR_WRITE_PROT_VIOLATION\n");
#endif
#ifdef HAL_SD_ERROR_LOCK_UNLOCK_FAILED
    if (err & HAL_SD_ERROR_LOCK_UNLOCK_FAILED)      printf("  HAL_SD_ERROR_LOCK_UNLOCK_FAILED\n");
#endif
#ifdef HAL_SD_ERROR_COM_CRC_FAILED
    if (err & HAL_SD_ERROR_COM_CRC_FAILED)          printf("  HAL_SD_ERROR_COM_CRC_FAILED\n");
#endif
#ifdef HAL_SD_ERROR_ILLEGAL_CMD
    if (err & HAL_SD_ERROR_ILLEGAL_CMD)             printf("  HAL_SD_ERROR_ILLEGAL_CMD\n");
#endif
#ifdef HAL_SD_ERROR_GENERAL_UNKNOWN_ERR
    if (err & HAL_SD_ERROR_GENERAL_UNKNOWN_ERR)     printf("  HAL_SD_ERROR_GENERAL_UNKNOWN_ERR\n");
#endif
#ifdef HAL_SD_ERROR_STREAM_READ_UNDERRUN
    if (err & HAL_SD_ERROR_STREAM_READ_UNDERRUN)    printf("  HAL_SD_ERROR_STREAM_READ_UNDERRUN\n");
#endif
#ifdef HAL_SD_ERROR_STREAM_WRITE_OVERRUN
    if (err & HAL_SD_ERROR_STREAM_WRITE_OVERRUN)    printf("  HAL_SD_ERROR_STREAM_WRITE_OVERRUN\n");
#endif
#ifdef HAL_SD_ERROR_TIMEOUT
    if (err & HAL_SD_ERROR_TIMEOUT)                 printf("  HAL_SD_ERROR_TIMEOUT\n");
#endif
#ifdef HAL_SD_ERROR_REQUEST_NOT_APPLICABLE
    if (err & HAL_SD_ERROR_REQUEST_NOT_APPLICABLE)  printf("  HAL_SD_ERROR_REQUEST_NOT_APPLICABLE\n");
#endif
#ifdef HAL_SD_ERROR_UNSUPPORTED_FEATURE
    if (err & HAL_SD_ERROR_UNSUPPORTED_FEATURE)     printf("  HAL_SD_ERROR_UNSUPPORTED_FEATURE\n");
#endif
#ifdef HAL_SD_ERROR_BUSY
    if (err & HAL_SD_ERROR_BUSY)                    printf("  HAL_SD_ERROR_BUSY\n");
#endif
#ifdef HAL_SD_ERROR_DMA
    if (err & HAL_SD_ERROR_DMA)                     printf("  HAL_SD_ERROR_DMA\n");
#endif
#ifdef HAL_SD_ERROR_INVALID_PARAMETER
    if (err & HAL_SD_ERROR_INVALID_PARAMETER)       printf("  HAL_SD_ERROR_INVALID_PARAMETER\n");
#endif
}

#if 0
extern SD_HandleTypeDef hsd2;

HAL_StatusTypeDef SD_ReadBlock(uint32_t lba, uint8_p buf) {
    // printf("SD_ReadBlock %lu\n", lba);
    HAL_StatusTypeDef st;

    // на всякий случай можно проверить состояние карты
#ifdef HAL_SD_CARD_TRANSFER
    if (HAL_SD_GetCardState(&hsd2) != HAL_SD_CARD_TRANSFER) printf("SD_ReadBlock: card not in TRANSFER state\n");

#endif

    st = HAL_SD_ReadBlocks(
        &hsd2,
        buf,
        lba,
        1,                  // one block
        SD_READ_TIMEOUT_MS
    );

    if (st != HAL_OK) {
        uint32_t err = HAL_SD_GetError(&hsd2);
        printf("SD_ReadBlock: HAL error, status=%d\n", (int)st);
        SD_PrintHalSdError(err);
    }

    return st;
}
#else
#include <string.h>    // memcpy

#define SD_BLOCK_SIZE         512U
#define SD_CACHE_LINES        4U

typedef struct {
    uint32_t lba;
    uint8_t  data[SD_BLOCK_SIZE];
    uint8_t  valid;
} SdCacheLine_t;

static SdCacheLine_t sd_cache[SD_CACHE_LINES];
static uint32_t sd_cache_next = 0;   // simple FIFO

static SdCacheLine_t* SD_CacheFind(uint32_t lba) {
    for (uint32_t i = 0; i < SD_CACHE_LINES; ++i) {
        if (sd_cache[i].valid && (sd_cache[i].lba == lba)) {
            return &sd_cache[i];
        }
    }
    return NULL;
}

static SdCacheLine_t* SD_CacheAllocLine(uint32_t lba) {
    SdCacheLine_t* line = &sd_cache[sd_cache_next];
    sd_cache_next++;
    if (sd_cache_next >= SD_CACHE_LINES) sd_cache_next = 0;

    line->lba = lba;
    line->valid = 0;
    return line;
}

extern SD_HandleTypeDef hsd2;

HAL_StatusTypeDef SD_ReadBlock(uint32_t lba, uint8_p buf) {
    HAL_StatusTypeDef st;
    SdCacheLine_t* line;

    // cache hit
    line = SD_CacheFind(lba);
    if (line != NULL) {
        // sector is cached
        memcpy(buf, line->data, SD_BLOCK_SIZE);
        return HAL_OK;
    }

    // cache miss: read from card into cache line
    line = SD_CacheAllocLine(lba);

#ifdef HAL_SD_CARD_TRANSFER
    if (HAL_SD_GetCardState(&hsd2) != HAL_SD_CARD_TRANSFER) {
        printf("SD_ReadBlock: card not in TRANSFER state\n");
    }
#endif

    st = HAL_SD_ReadBlocks(
        &hsd2,
        line->data,         // read directly into cache
        lba,
        1,
        SD_READ_TIMEOUT_MS
    );

    if (st != HAL_OK) {
        uint32_t err = HAL_SD_GetError(&hsd2);
        printf("SD_ReadBlock: HAL error, status=%d\n", (int)st);
        SD_PrintHalSdError(err);
        line->valid = 0;    // do not use this line
        return st;
    }

    line->valid = 1;
    memcpy(buf, line->data, SD_BLOCK_SIZE);
    return HAL_OK;
}

void SD_CacheInvalidate() {
    uint32_t i;
    for (i = 0; i < SD_CACHE_LINES; ++i) {
        sd_cache[i].valid = 0;
        sd_cache[i].lba = 0;
    }
    sd_cache_next = 0;
}
#endif

void SD_WaitCardReady() {
    HAL_SD_CardStateTypeDef state;
    uint32_t timeout = HAL_GetTick() + 1000; // 1s safety

    do {
        state = HAL_SD_GetCardState(&hsd2);
        if (HAL_GetTick() > timeout) {
            // log timeout, break or assert
            break;
        }
    } while (state != HAL_SD_CARD_TRANSFER);
}

HAL_SD_CardInfoTypeDef sd_info;

HAL_StatusTypeDef SD_DumpBlock(uint32_t lba) {
    uint32_t block_size = sd_info.LogBlockSize; // 512
    static uint8_t buf[2048];                   // с запасом, но хватит и 512

    if (block_size > sizeof(buf)) {
        printf("SD_DumpBlock: block_size=%lu too big for buffer\n",
            (uint32_t)block_size);
        return HAL_ERROR;
    }

    if (lba >= sd_info.LogBlockNbr) {
        printf("SD_DumpBlock: LBA %lu out of range (max %lu)\n",
            (uint32_t)lba,
            (uint32_t)(sd_info.LogBlockNbr - 1u));
        return HAL_ERROR;
    }

    HAL_StatusTypeDef st = SD_ReadBlock(lba, buf);
    if (st != HAL_OK) {
        printf("SD_DumpBlock: read error, status=%d\n", (int)st);
        return st;
    }

    printf("\n=== SD block %lu (size %lu bytes) ===\n",
        (uint32_t)lba,
        (uint32_t)block_size);

    for (uint32_t i = 0; i < block_size; i += 16) {
        // offset внутри блока
        printf("%04lX: ", (uint32_t)i);

        // hex колонка
        for (uint32_t j = 0; j < 16; ++j) {
            uint32_t idx = i + j;
            if (idx < block_size) {
                printf("%02X ", (unsigned int)buf[idx]);
            }
            else {
                printf("   ");
            }
        }

        printf(" ");

        // ASCII колонка
        for (uint32_t j = 0; j < 16; ++j) {
            uint32_t idx = i + j;
            char c = ' ';
            if (idx < block_size) {
                c = (char)buf[idx];
                if (c < 0x20 || c > 0x7E) {
                    c = '.';
                }
            }
            putchar(c);
        }

        printf("\n");
    }

    printf("=== end of block %lu ===\n\n", (uint32_t)lba);

    return HAL_OK;
}
