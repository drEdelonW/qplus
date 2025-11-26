#include <stdio.h>
// #include <stdint.h>
// #include "stm32f7xx_hal.h"
#include "perepherial.h"
#include "fs_FAT32.h"
#include "SD_TF.h"


SD_HandleTypeDef hsd2 = {
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


extern HAL_SD_CardInfoTypeDef sd_info;

uint8_t SD_InitAndGetInfo(void) {
    if (HAL_SD_Init(&hsd2) != HAL_OK)   return 1;   // init error


    // TODO: fix for 4 wire mode
    // if (HAL_SD_ConfigWideBusOperation(&hsd2, SDMMC_BUS_WIDE_4B) != HAL_OK) {
    //     return 2;   // bus config error
    // }

    if (HAL_SD_GetCardInfo(&hsd2, &sd_info) != HAL_OK) {
        return 3;   // card info error
    }

    // SD_PrintCardInfo(&sd_info);

    return 0;
}




uint32_t g_part1_lba_start = 0;
static FAT32_Volume_t g_fat32;

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
            (uint16_t)bytes_per_sector);
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
    printf("  part LBA start      = %lu\n", vol->part_lba_start);
    printf("  bytes/sector        = %u\n", vol->bytes_per_sector);
    printf("  sectors/cluster     = %u\n", vol->sectors_per_cluster);
    printf("  reserved sectors    = %u\n", vol->reserved_sectors);
    printf("  num FATs            = %u\n", vol->num_fats);
    printf("  FAT size (sectors)  = %lu\n", vol->fat_size_sectors);
    printf("  first data sector   = %lu (rel)\n", vol->first_data_sector);
    printf("  root cluster        = %lu\n", vol->root_cluster);

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
            (uint32_t)fat_lba);
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
                (uint32_t)lba);
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
static int FAT32_NameEquals(cStringRO a, cStringRO b) {
    while (*a && *b) {
        char ca = fat32_up(*a);
        char cb = fat32_up(*b);
        if (ca != cb) return 0;
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0');
}

static void FAT32_MakeShortName(cStringRO e, char* out, uint32_t out_size) {
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
    if (start_cluster < 2u)     return;

    // one cluster buffer (max 8 KB if SecPerClus up to 16)
    static uint8_t cluster_buf[512 * 16];

    uint32_t cluster = start_cluster;

    while (!FAT32_IsEOC(cluster)) {
        if (FAT32_ReadCluster(vol, cluster, cluster_buf) != 0) {
            return;
        }

        uint32_t bytes_in_cluster =
            (uint32_t)vol->bytes_per_sector * vol->sectors_per_cluster;

        for (uint32_t off = 0; (off + 32u) <= bytes_in_cluster; off += 32u) {
            cStringRO e = &cluster_buf[off];

            uint8_t first = e[0];
            if (first == 0x00)  break;  // no more entries in this directory
            if (first == 0xE5)  continue; // deleted

            uint8_t attr = e[11];

            if (attr == 0x0F)   continue; // LFN entry, skip
            if (attr & 0x08)    continue; // volume label


            char name[32];
            FAT32_MakeShortName(e, name, sizeof(name));

            uint16_t cl_hi = rd16_le(e + 20);
            uint16_t cl_lo = rd16_le(e + 26);
            uint32_t file_cluster = ((uint32_t)cl_hi << 16) | cl_lo;

            if (attr & 0x10) {
                // directory
                if (
                    (name[0] == '.') &&
                    (
                        (name[1] == '\0') ||
                        (
                            (name[1] == '.') &&
                            (name[2] == '\0')
                            )
                        )
                    ) {
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
        if (FAT32_IsEOC(next))  break;

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
    uint32_t            dir_cluster,
    cStringRO           name,
    uint8_t             want_dir,
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
            if (first == 0x00)  break;  // end of list
            if (first == 0xE5)  continue; // deleted


            uint8_t attr = e[11];

            if (attr == 0x0F)   continue; // LFN
            if (attr & 0x08)    continue; // volume label

            char short_name[32];
            FAT32_MakeShortName(e, short_name, sizeof(short_name));

            int is_dir = (attr & 0x10) ? 1 : 0;
            if (want_dir != is_dir) continue;

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
        if (FAT32_IsEOC(next))  break;
        cluster = next;
    }

    return -1; // not found
}

static cStringRO FAT32_NextPathToken(cStringRO path, char* token, uint32_t token_size) {
    // skip lead '/'
    while (*path == '/') path++;

    if (*path == '\0') {
        token[0] = '\0';
        return path;
    }

    uint32_t i = 0;
    while (
        (*path != '/') &&
        (*path != '\0') &&
        ((i + 1) < token_size)
        ) {
        token[i++] = *path++;
    }
    token[i] = '\0';

    // path now start with '/' or '\0'
    return path;
}

// found file by full path. then return cluster and size
int FAT32_FindPath(cStringRO path, FAT32_DirEntryInfo* out_file) {
    if (!path || !*path) return -1;

    FAT32_Volume_t* vol = &g_fat32;
    uint32_t current_cluster = vol->root_cluster;

    char token[32];
    cStringRO p = path;

    // проход по всем компонентам, кроме последнего
    while (1) {
        p = FAT32_NextPathToken(p, token, sizeof(token));
        if (token[0] == '\0')   break;  // пустой токен — конец

        // смотрим, есть ли ещё что-то после этого токена
        cStringRO p2 = p;
        while (*p2 == '/') { p2++; }
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
            if (out_file) { *out_file = info; }
            return 0;
        }
    }

    return -1;
}

#if 0
int FAT32_ReadFileToBuffer(cStringRO path, uint8_t* dst, uint32_t max_len, uint32_t* out_len) {
    FAT32_DirEntryInfo info;

    if (FAT32_FindPath(path, &info) != 0) {
        printf("FAT32_ReadFileToBuffer: file '%s' not found\n", path);
        return -1;
    }

    if (info.size > max_len) {
        printf("FAT32_ReadFileToBuffer: file '%s' too big (%lu bytes, max %lu)\n",
            path,
            (uint32_t)info.size,
            (uint32_t)max_len);
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


int FAT32_ReadFileToBuffer(cStringRO path, uint8_t* dst, uint32_t max_len, uint32_t* out_len) {
    FAT32_DirEntryInfo info;

    if (FAT32_FindPath(path, &info) != 0) {
        printf("FAT32_ReadFileToBuffer: file '%s' not found\n", path);
        return -1;
    }

    uint32_t want = info.size;
    if (want > max_len) {
        printf("FAT32_ReadFileToBuffer: file '%s' is %lu bytes, truncating to %lu\n",
            path,
            (uint32_t)info.size,
            (uint32_t)max_len);
        want = max_len;
    }

    FAT32_Volume_t* vol = &g_fat32;
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

        offset += to_copy;
        bytes_left -= to_copy;

        if (bytes_left == 0u) { break; }

        uint32_t next = FAT32_GetNextCluster(vol, cluster);
        if (FAT32_IsEOC(next))  break;
        cluster = next;
    }

    if (out_len) { *out_len = want; }

    return 0;
}
#endif

int FAT32_FileOpen(cStringRO path, FAT32_File_t* fh) {
    FAT32_DirEntryInfo info;

    if (!fh) return -1;

    if (FAT32_FindPath(path, &info) != 0) {
        printf("FAT32_FileOpen: file '%s' not found\n", path);
        return -1;
    }

    FAT32_Volume_t* vol = &g_fat32;

    fh->vol = vol;
    fh->first_cluster = info.cluster;
    fh->current_cluster = info.cluster;
    fh->file_size = info.size;
    fh->position = 0;
    fh->bytes_per_cluster =
        (uint32_t)vol->bytes_per_sector * vol->sectors_per_cluster;
    fh->cluster_index = 0;
    fh->cluster_valid = 0;

    return 0;
}

void FAT32_FileClose(FAT32_File_t* fh) {
    (void)fh; // do nothing for now
}

static int FAT32_FileLoadCluster(FAT32_File_t* fh) {
    if (!fh || !fh->vol) return -1;

    if ((fh->current_cluster < 2u) ||
        FAT32_IsEOC(fh->current_cluster)
        ) {
        return -1;
    }

    if (FAT32_ReadCluster(fh->vol, fh->current_cluster, fh->cluster_buf) != 0) {
        return -1;
    }

    fh->cluster_valid = 1;
    return 0;
}

int FAT32_FileRead(FAT32_File_t* fh, void* dst, uint32_t bytes_to_read, uint32_t* out_read) {
    if (!fh || !dst) return -1;

    uint8_t* out = (uint8_t*)dst;

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
        if ((!fh->cluster_valid) &&
            (FAT32_FileLoadCluster(fh) != 0))
            break;



        uint32_t pos_in_cluster = fh->position % fh->bytes_per_cluster;
        uint32_t left_in_cluster = fh->bytes_per_cluster - pos_in_cluster;

        uint32_t chunk = (bytes_to_read < left_in_cluster) ? bytes_to_read : left_in_cluster;

        // copy chunk
        for (uint32_t i = 0; i < chunk; ++i) {
            out[total_copied + i] = fh->cluster_buf[pos_in_cluster + i];
        }

        fh->position += chunk;
        total_copied += chunk;
        bytes_to_read -= chunk;

        if (chunk == left_in_cluster) {
            // перешли к следующему кластеру
            uint32_t next = FAT32_GetNextCluster(fh->vol, fh->current_cluster);
            if (FAT32_IsEOC(next)) {
                fh->current_cluster = next;
                fh->cluster_valid = 0;
                break;
            }
            else {
                fh->current_cluster = next;
                fh->cluster_valid = 0; // загрузим при следующей итерации
            }
        }
    }

    if (out_read) {
        *out_read = total_copied;
    }

    return 0;
}

int FAT32_FileRewind(FAT32_File_t* fh) {
    if (!fh) return -1;

    fh->current_cluster = fh->first_cluster;
    fh->position = 0;
    fh->cluster_index = 0;
    fh->cluster_valid = 0;

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

int SD_FS_Init(void) {
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

int FAT32_FileSeekSet(FAT32_File_t* fh, uint32_t new_pos) {
    if (!fh)    return -1;
    if (new_pos > fh->file_size)
        new_pos = fh->file_size;

    // если ищем в начало — можно дешево
    if (new_pos == 0)         return FAT32_FileRewind(fh);

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
    SD_FileSlot* slot = SDFS_GetSlot(handle);
    if (!slot) return -1;

    if (FAT32_FileSeekSet(&slot->file, (uint32_t)position) != 0) {
        return -1;
    }
    return 0;
}



void MX_SDMMC2_SD_Init() {
    if (SD_InitAndGetInfo() == 0) {
        printf("SD inited\n");
        SD_PrintMBR();

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
    }
    else {
        printf("SD ERROR\n");
    }
}