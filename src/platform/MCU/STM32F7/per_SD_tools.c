#include <stdio.h>
// #include <stdint.h>
// #include "stm32f7xx_hal.h"
#include "perepherial.h"



static const char* SD_GetCardTypeStr(uint32_t t) {
#ifdef CARD_SDHC_SDXC
    if (t == CARD_SDHC_SDXC) return "SDHC / SDXC";
#endif
#ifdef CARD_SDSC
    if (t == CARD_SDSC) return "SDSC";
#endif
#ifdef CARD_SECURED
    if (t == CARD_SECURED) return "Secured";
#endif
#ifdef CARD_MMC
    if (t == CARD_MMC) return "MMC";
#endif
    return "Unknown";
}

static const char* SD_GetCardVersionStr(uint32_t v) {
#ifdef CARD_V1_X
    if (v == CARD_V1_X) return "SD v1.x";
#endif
#ifdef CARD_V2_X
    if (v == CARD_V2_X) return "SD v2.x / SDHC / SDXC";
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

static SD_HandleTypeDef hsd2 = {
    .Instance = SDMMC2,
    .Init = {
        .ClockEdge = SDMMC_CLOCK_EDGE_RISING,
        .ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE,
        .ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE,
        .BusWide = SDMMC_BUS_WIDE_1B,
        .HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE,
        .ClockDiv = 0,
    }
};

static uint32_t g_part1_lba_start = 0;

typedef struct {
    uint32_t part_lba_start;
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint32_t fat_size_sectors;
    uint32_t root_cluster;
    uint32_t first_data_sector;  // relative to partition start
    uint32_t fat_start_lba;      // absolute LBA
} FAT32_Volume_t;
typedef struct {
    FAT32_Volume_t *vol;

    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t file_size;
    uint32_t position;           // общее смещение от начала файла

    uint32_t bytes_per_cluster;

    uint32_t cluster_index;      // номер кластера в цепочке (0,1,2,...)
    uint8_t  cluster_valid;      // флаг, что cluster_buf актуален

    uint8_t  cluster_buf[512 * 16]; // один кластер (512 * sectors_per_cluster)
} FAT32_File_t;
static FAT32_Volume_t g_fat32;
static uint16_t rd16_le(const uint8_p p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t rd32_le(const uint8_p p) {
    return  (uint32_t)p[0]
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);
}
HAL_SD_CardInfoTypeDef sd_info;

uint8_t SD_InitAndGetInfo(void) {
    if (HAL_SD_Init(&hsd2) != HAL_OK) {
        return 1;   // init error
    }

    // TODO: fix for 4 wire mode
    // if (HAL_SD_ConfigWideBusOperation(&hsd2, SDMMC_BUS_WIDE_4B) != HAL_OK) {
    //     return 2;   // bus config error
    // }

    if (HAL_SD_GetCardInfo(&hsd2, &sd_info) != HAL_OK) {
        return 3;   // card info error
    }

    return 0;
}


#define SD_READ_TIMEOUT_MS  1000

static void SD_PrintHalSdError(uint32_t err) {
    printf("SD error mask: 0x%08lX\n", (uint32_t)err);

#ifdef HAL_SD_ERROR_NONE
    if (err == HAL_SD_ERROR_NONE) {
        printf("  HAL_SD_ERROR_NONE\n");
        return;
    }
#endif

#ifdef HAL_SD_ERROR_ADDR_OUT_OF_RANGE
    if (err & HAL_SD_ERROR_ADDR_OUT_OF_RANGE) {
        printf("  HAL_SD_ERROR_ADDR_OUT_OF_RANGE\n");
    }
#endif
#ifdef HAL_SD_ERROR_ADDR_MISALIGNED
    if (err & HAL_SD_ERROR_ADDR_MISALIGNED) {
        printf("  HAL_SD_ERROR_ADDR_MISALIGNED\n");
    }
#endif
#ifdef HAL_SD_ERROR_BLOCK_LEN_ERR
    if (err & HAL_SD_ERROR_BLOCK_LEN_ERR) {
        printf("  HAL_SD_ERROR_BLOCK_LEN_ERR\n");
    }
#endif
#ifdef HAL_SD_ERROR_ERASE_SEQ_ERR
    if (err & HAL_SD_ERROR_ERASE_SEQ_ERR) {
        printf("  HAL_SD_ERROR_ERASE_SEQ_ERR\n");
    }
#endif
#ifdef HAL_SD_ERROR_BAD_CID
    if (err & HAL_SD_ERROR_BAD_CID) {
        printf("  HAL_SD_ERROR_BAD_CID\n");
    }
#endif
#ifdef HAL_SD_ERROR_WRITE_PROT_VIOLATION
    if (err & HAL_SD_ERROR_WRITE_PROT_VIOLATION) {
        printf("  HAL_SD_ERROR_WRITE_PROT_VIOLATION\n");
    }
#endif
#ifdef HAL_SD_ERROR_LOCK_UNLOCK_FAILED
    if (err & HAL_SD_ERROR_LOCK_UNLOCK_FAILED) {
        printf("  HAL_SD_ERROR_LOCK_UNLOCK_FAILED\n");
    }
#endif
#ifdef HAL_SD_ERROR_COM_CRC_FAILED
    if (err & HAL_SD_ERROR_COM_CRC_FAILED) {
        printf("  HAL_SD_ERROR_COM_CRC_FAILED\n");
    }
#endif
#ifdef HAL_SD_ERROR_ILLEGAL_CMD
    if (err & HAL_SD_ERROR_ILLEGAL_CMD) {
        printf("  HAL_SD_ERROR_ILLEGAL_CMD\n");
    }
#endif
#ifdef HAL_SD_ERROR_GENERAL_UNKNOWN_ERR
    if (err & HAL_SD_ERROR_GENERAL_UNKNOWN_ERR) {
        printf("  HAL_SD_ERROR_GENERAL_UNKNOWN_ERR\n");
    }
#endif
#ifdef HAL_SD_ERROR_STREAM_READ_UNDERRUN
    if (err & HAL_SD_ERROR_STREAM_READ_UNDERRUN) {
        printf("  HAL_SD_ERROR_STREAM_READ_UNDERRUN\n");
    }
#endif
#ifdef HAL_SD_ERROR_STREAM_WRITE_OVERRUN
    if (err & HAL_SD_ERROR_STREAM_WRITE_OVERRUN) {
        printf("  HAL_SD_ERROR_STREAM_WRITE_OVERRUN\n");
    }
#endif
#ifdef HAL_SD_ERROR_TIMEOUT
    if (err & HAL_SD_ERROR_TIMEOUT) {
        printf("  HAL_SD_ERROR_TIMEOUT\n");
    }
#endif
#ifdef HAL_SD_ERROR_REQUEST_NOT_APPLICABLE
    if (err & HAL_SD_ERROR_REQUEST_NOT_APPLICABLE) {
        printf("  HAL_SD_ERROR_REQUEST_NOT_APPLICABLE\n");
    }
#endif
#ifdef HAL_SD_ERROR_UNSUPPORTED_FEATURE
    if (err & HAL_SD_ERROR_UNSUPPORTED_FEATURE) {
        printf("  HAL_SD_ERROR_UNSUPPORTED_FEATURE\n");
    }
#endif
#ifdef HAL_SD_ERROR_BUSY
    if (err & HAL_SD_ERROR_BUSY) {
        printf("  HAL_SD_ERROR_BUSY\n");
    }
#endif
#ifdef HAL_SD_ERROR_DMA
    if (err & HAL_SD_ERROR_DMA) {
        printf("  HAL_SD_ERROR_DMA\n");
    }
#endif
#ifdef HAL_SD_ERROR_INVALID_PARAMETER
    if (err & HAL_SD_ERROR_INVALID_PARAMETER) {
        printf("  HAL_SD_ERROR_INVALID_PARAMETER\n");
    }
#endif
}

HAL_StatusTypeDef SD_ReadBlock(uint32_t lba, uint8_p buf) {
    HAL_StatusTypeDef st;

    // на всякий случай можно проверить состояние карты
#ifdef HAL_SD_CARD_TRANSFER
    if (HAL_SD_GetCardState(&hsd2) != HAL_SD_CARD_TRANSFER) {
        printf("SD_ReadBlock: card not in TRANSFER state\n");
    }
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


static const char* MBR_PartTypeStr(uint8_t type) {
    switch (type) {
    case 0x01: return "FAT12";
    case 0x04: return "FAT16 <32M";
    case 0x06: return "FAT16";
    case 0x07: return "NTFS/exFAT";
    case 0x0B: return "FAT32 CHS";
    case 0x0C: return "FAT32 LBA";
    case 0x0E: return "FAT16 LBA";
    case 0x0F: return "Ext LBA";
    case 0x82: return "Linux swap";
    case 0x83: return "Linux";
    case 0xEE: return "GPT protective";
    default:   return "Unknown";
    }
}

void SD_PrintMBR(void) {
    uint8_t buf[512];

    if (SD_ReadBlock(0, buf) != HAL_OK) {
        printf("SD_PrintMBR: can't read LBA0\n");
        return;
    }

    uint8_t sig_lo = buf[510];
    uint8_t sig_hi = buf[511];
    if (sig_lo != 0x55 || sig_hi != 0xAA) {
        printf("SD_PrintMBR: no valid MBR signature (0x%02X 0x%02X)\n",
            (unsigned int)sig_lo, (unsigned int)sig_hi);
        return;
    }

    printf("=== MBR Partition Table ===\n");

    g_part1_lba_start = 0;

    for (uint32_t i = 0; i < 4u; ++i) {
        const uint32_t off = 0x1BEu + i * 16u;
        const uint8_p e = &buf[off];

        uint8_t  status = e[0];       // 0x80 = bootable
        uint8_t  type = e[4];
        uint32_t lba_first = rd32_le(e + 8);
        uint32_t sectors = rd32_le(e + 12);

        if (type == 0 && lba_first == 0 && sectors == 0) {
            continue; // empty entry
        }

        uint64_t bytes = (uint64_t)sectors * 512u;
        uint32_t mb = (uint32_t)(bytes / (1024u * 1024u));

        printf("Part %lu:\n", (unsigned long)(i + 1u));
        printf("  Boot      : %s (0x%02X)\n",
            (status == 0x80) ? "yes" : "no",
            (unsigned int)status);
        printf("  Type      : 0x%02X (%s)\n",
            (unsigned int)type,
            MBR_PartTypeStr(type));
        printf("  LBA start : %lu\n",
            (unsigned long)lba_first);
        printf("  Sectors   : %lu\n",
            (unsigned long)sectors);
        printf("  Size      : %lu MB (approx)\n",
            (unsigned long)mb);

        if (g_part1_lba_start == 0 && (type == 0x0B || type == 0x0C)) {
            g_part1_lba_start = lba_first;
        }
    }

    printf("===========================\n");
    printf("FAT32 partition LBA start = %lu\n", (unsigned long)g_part1_lba_start);
}

static int FAT32_Mount(FAT32_Volume_t* vol) {
    if (g_part1_lba_start == 0) {
        printf("FAT32_Mount: no FAT32 partition LBA\n");
        return -1;
    }

    uint8_t sector[512];
    if (SD_ReadBlock(g_part1_lba_start, sector) != HAL_OK) {
        printf("FAT32_Mount: cannot read boot sector\n");
        return -1;
    }

    uint16_t bytes_per_sector = rd16_le(&sector[11]); // BPB_BytsPerSec
    uint8_t  sec_per_clus = sector[13];           // BPB_SecPerClus
    uint16_t rsvd_sec_cnt = rd16_le(&sector[14]); // BPB_RsvdSecCnt
    uint8_t  num_fats = sector[16];           // BPB_NumFATs
    uint32_t fatsz32 = rd32_le(&sector[36]); // BPB_FATSz32
    uint32_t root_clus = rd32_le(&sector[44]); // BPB_RootClus

    if (bytes_per_sector != 512u) {
        printf("FAT32_Mount: unsupported bytes_per_sector=%u\n",
            (unsigned int)bytes_per_sector);
        return -1;
    }

    uint32_t first_data_sector =
        (uint32_t)rsvd_sec_cnt + ((uint32_t)num_fats * fatsz32);

    vol->part_lba_start = g_part1_lba_start;
    vol->bytes_per_sector = bytes_per_sector;
    vol->sectors_per_cluster = sec_per_clus;
    vol->reserved_sectors = rsvd_sec_cnt;
    vol->num_fats = num_fats;
    vol->fat_size_sectors = fatsz32;
    vol->root_cluster = root_clus;
    vol->first_data_sector = first_data_sector;
    vol->fat_start_lba = g_part1_lba_start + rsvd_sec_cnt;

    printf("FAT32 mounted:\n");
    printf("  part LBA start      = %lu\n", (unsigned long)vol->part_lba_start);
    printf("  bytes/sector        = %u\n", (unsigned int)vol->bytes_per_sector);
    printf("  sectors/cluster     = %u\n", (unsigned int)vol->sectors_per_cluster);
    printf("  reserved sectors    = %u\n", (unsigned int)vol->reserved_sectors);
    printf("  num FATs            = %u\n", (unsigned int)vol->num_fats);
    printf("  FAT size (sectors)  = %lu\n", (unsigned long)vol->fat_size_sectors);
    printf("  first data sector   = %lu (rel)\n", (unsigned long)vol->first_data_sector);
    printf("  root cluster        = %lu\n", (unsigned long)vol->root_cluster);

    return 0;
}

static uint32_t FAT32_GetNextCluster(FAT32_Volume_t* vol, uint32_t cluster) {
    uint32_t fat_offset = cluster * 4u; // 4 bytes per FAT32 entry
    uint32_t fat_sector_idx = fat_offset / vol->bytes_per_sector;
    uint32_t ent_offset = fat_offset % vol->bytes_per_sector;

    uint8_t sector[512];
    uint32_t fat_lba = vol->fat_start_lba + fat_sector_idx;

    if (SD_ReadBlock(fat_lba, sector) != HAL_OK) {
        printf("FAT32_GetNextCluster: read error at LBA %lu\n",
            (unsigned long)fat_lba);
        return 0x0FFFFFFF; // treat as end
    }

    uint32_t val = rd32_le(&sector[ent_offset]);
    val &= 0x0FFFFFFF; // FAT32 uses low 28 bits

    return val;
}

static int FAT32_IsEOC(uint32_t cl) {
    return (cl >= 0x0FFFFFF8u);
}

static int FAT32_ReadCluster(FAT32_Volume_t* vol, uint32_t cluster, uint8_p buf) {
    uint32_t first_sector_of_cluster =
        vol->first_data_sector + (cluster - 2u) * (uint32_t)vol->sectors_per_cluster;

    for (uint32_t i = 0; i < vol->sectors_per_cluster; ++i) {
        uint32_t lba = vol->part_lba_start + first_sector_of_cluster + i;
        if (SD_ReadBlock(lba, buf + i * vol->bytes_per_sector) != HAL_OK) {
            printf("FAT32_ReadCluster: read error at LBA %lu\n",
                (unsigned long)lba);
            return -1;
        }
    }
    return 0;
}

static void FAT32_PrintIndent(int depth) {
    for (int i = 0; i < depth; ++i) {
        printf("  ");
    }
}

static char fat32_up(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

// сравнение имени (из каталога) и запрошенного, без учёта регистра
static int FAT32_NameEquals(const char* a, const char* b) {
    while (*a && *b) {
        char ca = fat32_up(*a);
        char cb = fat32_up(*b);
        if (ca != cb) return 0;
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0');
}

static void FAT32_MakeShortName(const uint8_p e, char* out, uint32_t out_size) {
    // e[0..7] - name, e[8..10] - ext
    char name[9];
    char ext[4];

    for (int i = 0; i < 8; ++i) name[i] = (char)e[i];
    name[8] = '\0';
    for (int i = 0; i < 3; ++i) ext[i] = (char)e[8 + i];
    ext[3] = '\0';

    // trim spaces
    int end = 7;
    while (end >= 0 && name[end] == ' ') {
        name[end] = '\0';
        --end;
    }
    end = 2;
    while (end >= 0 && ext[end] == ' ') {
        ext[end] = '\0';
        --end;
    }

    if (ext[0] != '\0') {
        snprintf(out, out_size, "%s.%s", name, ext);
    }
    else {
        snprintf(out, out_size, "%s", name);
    }
}

static void FAT32_ListDir(FAT32_Volume_t* vol, uint32_t start_cluster, int depth) {
    if (start_cluster < 2u) {
        return;
    }

    // one cluster buffer (max 8 KB if SecPerClus up to 16)
    static uint8_t cluster_buf[512 * 16];

    uint32_t cluster = start_cluster;

    while (!FAT32_IsEOC(cluster)) {
        if (FAT32_ReadCluster(vol, cluster, cluster_buf) != 0) {
            return;
        }

        uint32_t bytes_in_cluster =
            (uint32_t)vol->bytes_per_sector * vol->sectors_per_cluster;

        for (uint32_t off = 0; off + 32u <= bytes_in_cluster; off += 32u) {
            const uint8_p e = &cluster_buf[off];

            uint8_t first = e[0];
            if (first == 0x00) {
                // no more entries in this directory
                break;
            }
            if (first == 0xE5) {
                continue; // deleted
            }

            uint8_t attr = e[11];

            if (attr == 0x0F) {
                continue; // LFN entry, skip
            }
            if (attr & 0x08) {
                continue; // volume label
            }

            char name[32];
            FAT32_MakeShortName(e, name, sizeof(name));

            uint16_t cl_hi = rd16_le(e + 20);
            uint16_t cl_lo = rd16_le(e + 26);
            uint32_t file_cluster = ((uint32_t)cl_hi << 16) | cl_lo;

            if (attr & 0x10) {
                // directory
                if ((name[0] == '.') &&
                    ((name[1] == '\0') ||
                        (name[1] == '.' && name[2] == '\0'))) {
                    // skip . and ..
                    continue;
                }

                FAT32_PrintIndent(depth);
                printf("[%s]\n", name);

                FAT32_ListDir(vol, file_cluster, depth + 1);
            }
            else {
                // regular file
                FAT32_PrintIndent(depth);
                printf("%s\n", name);
            }
        }

        uint32_t next = FAT32_GetNextCluster(vol, cluster);
        if (FAT32_IsEOC(next)) {
            break;
        }
        cluster = next;
    }
}

void FAT32_PrintTree() {
    if (FAT32_Mount(&g_fat32) != 0) {
        printf("FAT32_PrintTree: mount failed\n");
        return;
    }

    printf("=== FAT32 directory tree ===\n");
    FAT32_ListDir(&g_fat32, g_fat32.root_cluster, 0);
    printf("=== end of tree ===\n");
}

typedef struct {
    uint32_t cluster;
    uint32_t size;
    uint8_t  attr;
} FAT32_DirEntryInfo;

static int FAT32_FindInDir(
    FAT32_Volume_t* vol,
    uint32_t dir_cluster,
    const char* name,
    uint8_t want_dir,
    FAT32_DirEntryInfo* out) {
    if (dir_cluster < 2u) return -1;

    static uint8_t cluster_buf[512 * 16]; // как в ListDir
    uint32_t cluster = dir_cluster;

    while (!FAT32_IsEOC(cluster)) {
        if (FAT32_ReadCluster(vol, cluster, cluster_buf) != 0) {
            return -1;
        }

        uint32_t bytes_in_cluster =
            (uint32_t)vol->bytes_per_sector * vol->sectors_per_cluster;

        for (uint32_t off = 0; off + 32u <= bytes_in_cluster; off += 32u) {
            const uint8_t* e = &cluster_buf[off];

            uint8_t first = e[0];
            if (first == 0x00) {
                // конец списка
                break;
            }
            if (first == 0xE5) {
                continue; // deleted
            }

            uint8_t attr = e[11];

            if (attr == 0x0F) {
                continue; // LFN
            }
            if (attr & 0x08) {
                continue; // volume label
            }

            char short_name[32];
            FAT32_MakeShortName(e, short_name, sizeof(short_name));

            int is_dir = (attr & 0x10) ? 1 : 0;
            if (want_dir != is_dir) {
                continue;
            }

            if (!FAT32_NameEquals(short_name, name)) {
                continue;
            }

            uint16_t cl_hi = rd16_le(e + 20);
            uint16_t cl_lo = rd16_le(e + 26);
            uint32_t file_cluster = ((uint32_t)cl_hi << 16) | cl_lo;
            uint32_t file_size = rd32_le(e + 28);

            if (out) {
                out->cluster = file_cluster;
                out->size = file_size;
                out->attr = attr;
            }
            return 0;
        }

        uint32_t next = FAT32_GetNextCluster(vol, cluster);
        if (FAT32_IsEOC(next)) {
            break;
        }
        cluster = next;
    }

    return -1; // не найдено
}

static const char* FAT32_NextPathToken(const char* path, char* token, uint32_t token_size) {
    // пропускаем ведущие '/'
    while (*path == '/') path++;

    if (*path == '\0') {
        token[0] = '\0';
        return path;
    }

    uint32_t i = 0;
    while (*path != '/' && *path != '\0' && i + 1 < token_size) {
        token[i++] = *path++;
    }
    token[i] = '\0';

    // path сейчас либо на '/', либо на '\0'
    return path;
}

// найти файл по полному пути, вернуть кластер и размер
int FAT32_FindPath(const char* path, FAT32_DirEntryInfo* out_file) {
    if (!path || !*path) return -1;

    FAT32_Volume_t* vol = &g_fat32;
    uint32_t current_cluster = vol->root_cluster;

    char token[32];
    const char* p = path;

    // проход по всем компонентам, кроме последнего
    while (1) {
        p = FAT32_NextPathToken(p, token, sizeof(token));
        if (token[0] == '\0') {
            // пустой токен — конец
            break;
        }

        // смотрим, есть ли ещё что-то после этого токена
        const char* p2 = p;
        while (*p2 == '/') p2++;
        int last = (*p2 == '\0');

        FAT32_DirEntryInfo info;
        if (!last) {
            // промежуточный элемент — директория
            if (FAT32_FindInDir(vol, current_cluster, token, 1, &info) != 0) {
                printf("FAT32_FindPath: dir '%s' not found\n", token);
                return -1;
            }
            current_cluster = info.cluster;
        }
        else {
            // последний элемент — файл
            if (FAT32_FindInDir(vol, current_cluster, token, 0, &info) != 0) {
                printf("FAT32_FindPath: file '%s' not found\n", token);
                return -1;
            }
            if (out_file) {
                *out_file = info;
            }
            return 0;
        }
    }

    return -1;
}
#if 0
int FAT32_ReadFileToBuffer(const char* path, uint8_t* dst, uint32_t max_len, uint32_t* out_len) {
    FAT32_DirEntryInfo info;

    if (FAT32_FindPath(path, &info) != 0) {
        printf("FAT32_ReadFileToBuffer: file '%s' not found\n", path);
        return -1;
    }

    if (info.size > max_len) {
        printf("FAT32_ReadFileToBuffer: file '%s' too big (%lu bytes, max %lu)\n",
            path,
            (unsigned long)info.size,
            (unsigned long)max_len);
        return -1;
    }

    FAT32_Volume_t* vol = &g_fat32;
    uint32_t cluster = info.cluster;
    uint32_t bytes_per_cluster =
        (uint32_t)vol->bytes_per_sector * vol->sectors_per_cluster;

    uint32_t bytes_left = info.size;
    uint32_t offset = 0;

    static uint8_t cluster_buf[512 * 16];

    while (!FAT32_IsEOC(cluster) && bytes_left > 0u) {
        if (FAT32_ReadCluster(vol, cluster, cluster_buf) != 0) {
            printf("FAT32_ReadFileToBuffer: read cluster error\n");
            return -1;
        }

        uint32_t to_copy = (bytes_left < bytes_per_cluster) ? bytes_left : bytes_per_cluster;

        for (uint32_t i = 0; i < to_copy; ++i) {
            dst[offset + i] = cluster_buf[i];
        }

        offset += to_copy;
        bytes_left -= to_copy;

        if (bytes_left == 0u) {
            break;
        }

        uint32_t next = FAT32_GetNextCluster(vol, cluster);
        if (FAT32_IsEOC(next)) {
            break;
        }
        cluster = next;
    }

    if (out_len) {
        *out_len = info.size;
    }

    return 0;
}
#else
int FAT32_ReadFileToBuffer(const char *path, uint8_t *dst, uint32_t max_len, uint32_t *out_len) {
    FAT32_DirEntryInfo info;

    if (FAT32_FindPath(path, &info) != 0) {
        printf("FAT32_ReadFileToBuffer: file '%s' not found\n", path);
        return -1;
    }

    uint32_t want = info.size;
    if (want > max_len) {
        printf("FAT32_ReadFileToBuffer: file '%s' is %lu bytes, truncating to %lu\n",
               path,
               (unsigned long)info.size,
               (unsigned long)max_len);
        want = max_len;
    }

    FAT32_Volume_t *vol = &g_fat32;
    uint32_t cluster = info.cluster;
    uint32_t bytes_per_cluster =
        (uint32_t)vol->bytes_per_sector * vol->sectors_per_cluster;

    uint32_t bytes_left = want;
    uint32_t offset = 0;

    static uint8_t cluster_buf[512 * 16];

    while (!FAT32_IsEOC(cluster) && bytes_left > 0u) {
        if (FAT32_ReadCluster(vol, cluster, cluster_buf) != 0) {
            printf("FAT32_ReadFileToBuffer: read cluster error\n");
            return -1;
        }

        uint32_t to_copy = (bytes_left < bytes_per_cluster) ? bytes_left : bytes_per_cluster;

        for (uint32_t i = 0; i < to_copy; ++i) {
            dst[offset + i] = cluster_buf[i];
        }

        offset     += to_copy;
        bytes_left -= to_copy;

        if (bytes_left == 0u) {
            break;
        }

        uint32_t next = FAT32_GetNextCluster(vol, cluster);
        if (FAT32_IsEOC(next)) {
            break;
        }
        cluster = next;
    }

    if (out_len) {
        *out_len = want;
    }

    return 0;
}
#endif

int FAT32_FileOpen(const char *path, FAT32_File_t *fh) {
    FAT32_DirEntryInfo info;

    if (!fh) return -1;

    if (FAT32_FindPath(path, &info) != 0) {
        printf("FAT32_FileOpen: file '%s' not found\n", path);
        return -1;
    }

    FAT32_Volume_t *vol = &g_fat32;

    fh->vol              = vol;
    fh->first_cluster    = info.cluster;
    fh->current_cluster  = info.cluster;
    fh->file_size        = info.size;
    fh->position         = 0;
    fh->bytes_per_cluster =
        (uint32_t)vol->bytes_per_sector * vol->sectors_per_cluster;
    fh->cluster_index    = 0;
    fh->cluster_valid    = 0;

    return 0;
}

void FAT32_FileClose(FAT32_File_t *fh) {
    (void)fh; // сейчас ничего не делаем
}

static int FAT32_FileLoadCluster(FAT32_File_t *fh) {
    if (!fh || !fh->vol) return -1;

    if (fh->current_cluster < 2u || FAT32_IsEOC(fh->current_cluster)) {
        return -1;
    }

    if (FAT32_ReadCluster(fh->vol, fh->current_cluster, fh->cluster_buf) != 0) {
        return -1;
    }

    fh->cluster_valid = 1;
    return 0;
}

int FAT32_FileRead(FAT32_File_t *fh, void *dst, uint32_t bytes_to_read, uint32_t *out_read) {
    if (!fh || !dst) return -1;

    uint8_t *out = (uint8_t *)dst;

    if (fh->position >= fh->file_size) {
        if (out_read) *out_read = 0;
        return 0; // EOF
    }

    uint32_t remaining = fh->file_size - fh->position;
    if (bytes_to_read > remaining) {
        bytes_to_read = remaining;
    }

    uint32_t total_copied = 0;

    while (bytes_to_read > 0) {
        if (!fh->cluster_valid) {
            if (FAT32_FileLoadCluster(fh) != 0) {
                break;
            }
        }

        uint32_t pos_in_cluster = fh->position % fh->bytes_per_cluster;
        uint32_t left_in_cluster = fh->bytes_per_cluster - pos_in_cluster;

        uint32_t chunk = (bytes_to_read < left_in_cluster) ? bytes_to_read : left_in_cluster;

        // copy chunk
        for (uint32_t i = 0; i < chunk; ++i) {
            out[total_copied + i] = fh->cluster_buf[pos_in_cluster + i];
        }

        fh->position   += chunk;
        total_copied   += chunk;
        bytes_to_read  -= chunk;

        if (chunk == left_in_cluster) {
            // перешли к следующему кластеру
            uint32_t next = FAT32_GetNextCluster(fh->vol, fh->current_cluster);
            if (FAT32_IsEOC(next)) {
                fh->current_cluster = next;
                fh->cluster_valid   = 0;
                break;
            } else {
                fh->current_cluster = next;
                fh->cluster_valid   = 0; // загрузим при следующей итерации
            }
        }
    }

    if (out_read) {
        *out_read = total_copied;
    }

    return 0;
}

int FAT32_FileRewind(FAT32_File_t *fh) {
    if (!fh) return -1;

    fh->current_cluster  = fh->first_cluster;
    fh->position         = 0;
    fh->cluster_index    = 0;
    fh->cluster_valid    = 0;

    return 0;
}

static void SDFS_MakeFatPath(const char *quakePath, char *out, uint32_t outSize) {
    // базовый префикс для карты
    const char *base = "QUAKE/";
    uint32_t i = 0;

    // копируем префикс
    while (*base && i + 1 < outSize) {
        out[i++] = *base++;
    }

    // копируем исходный путь, поднимая регистр и заменяя '\' на '/'
    for (const char *p = quakePath; *p && i + 1 < outSize; ++p) {
        char c = *p;
        if (c == '\\') c = '/';
        // uppercase латиницу
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        out[i++] = c;
    }

    out[i] = '\0';
}

#define SDFS_MAX_OPEN_FILES 8

typedef struct {
    int           used;
    FAT32_File_t  file;
} SD_FileSlot;

static SD_FileSlot s_sdFiles[SDFS_MAX_OPEN_FILES];

static int SDFS_AllocHandle(void) {
    for (int i = 0; i < SDFS_MAX_OPEN_FILES; ++i) {
        if (!s_sdFiles[i].used) {
            s_sdFiles[i].used = 1;
            return i;
        }
    }
    return -1;
}

static SD_FileSlot* SDFS_GetSlot(int hnd) {
    if (hnd < 0 || hnd >= SDFS_MAX_OPEN_FILES) return NULL;
    if (!s_sdFiles[hnd].used) return NULL;
    return &s_sdFiles[hnd];
}

static int s_sdfs_inited = 0;

int SD_FS_Init(void) {
    if (s_sdfs_inited) return 0;

    if (SD_InitAndGetInfo() != 0) {
        printf("SD_FS_Init: SD init failed\n");
        return -1;
    }

    SD_PrintCardInfo(&sd_info);
    SD_PrintMBR();

    if (FAT32_Mount(&g_fat32) != 0) {
        printf("SD_FS_Init: FAT32 mount failed\n");
        return -1;
    }

    s_sdfs_inited = 1;
    return 0;
}

int Sys_FileOpenRead(const char *quakePath, int *hnd) {
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
    SD_FileSlot *slot = SDFS_GetSlot(handle);
    if (!slot) return;

    FAT32_FileClose(&slot->file);
    slot->used = 0;
}

int Sys_FileRead(int handle, void *dest, int count) {
    SD_FileSlot *slot = SDFS_GetSlot(handle);
    if (!slot) return 0;

    uint32_t got = 0;
    if (FAT32_FileRead(&slot->file, dest, (uint32_t)count, &got) != 0) {
        return 0;
    }
    return (int)got;
}

int FAT32_FileSeekSet(FAT32_File_t *fh, uint32_t new_pos) {
    if (!fh) return -1;
    if (new_pos > fh->file_size) new_pos = fh->file_size;

    // если ищем в начало — можно дешево
    if (new_pos == 0) {
        return FAT32_FileRewind(fh);
    }

    // грубо: перемотать в начало и прочитать мусор до new_pos
    FAT32_FileRewind(fh);

    uint8_t tmp[64];
    uint32_t left = new_pos;
    while (left > 0) {
        uint32_t chunk = (left > sizeof(tmp)) ? (uint32_t)sizeof(tmp) : left;
        uint32_t got = 0;
        if (FAT32_FileRead(fh, tmp, chunk, &got) != 0) {
            return -1;
        }
        if (got == 0) break;
        left -= got;
    }

    return 0;
}
int Sys_FileSeek(int handle, int position) {
    SD_FileSlot *slot = SDFS_GetSlot(handle);
    if (!slot) return -1;

    if (FAT32_FileSeekSet(&slot->file, (uint32_t)position) != 0) {
        return -1;
    }
    return 0;
}
void MX_SDMMC2_SD_Init() {
    if (SD_InitAndGetInfo() == 0) {
        printf("SD inited\n");
        SD_PrintCardInfo(&sd_info);
        SD_PrintMBR();

        if (FAT32_Mount(&g_fat32) == 0) {
            // FAT32_PrintTree();

            // FAT32_File_t f;
            // if (FAT32_FileOpen("QUAKE/ID1/PAK0.PAK", &f) == 0) {
            //     printf("PAK0.PAK size: %lu bytes\n", (unsigned long)f.file_size);

            //     uint8_t header[64];
            //     uint32_t got = 0;
            //     if (FAT32_FileRead(&f, header, sizeof(header), &got) == 0) {
            //         printf("Read first %lu bytes:\n", (unsigned long)got);
            //         for (uint32_t i = 0; i < got; i += 16) {
            //             printf("%04lX: ", (unsigned long)i);
            //             for (uint32_t j = 0; j < 16 && i + j < got; ++j) {
            //                 printf("%02X ", (unsigned int)header[i + j]);
            //             }
            //             printf("\n");
            //         }
            //     }

            //     FAT32_FileClose(&f);
            // }
        }
    }
    else {
        printf("SD ERROR\n");
    }
}