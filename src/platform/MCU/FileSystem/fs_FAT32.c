#include "MBR.h"
#include "SD_TF.h"
#include "fs_FAT32.h"
#include "terminal_tools.h"

#pragma pack(push, 1)
typedef struct {
    uint8_t  jmpBoot[3];          // 0x00 Jump instruction
    char     OEMName[8];          // 0x03 OEM string

    uint16_t BytsPerSec;          // 0x0B Bytes per sector (512, 1024...)
    uint8_t  SecPerClus;          // 0x0D Sectors per cluster (1,2,4,8,16…)
    uint16_t RsvdSecCnt;          // 0x0E Reserved sectors (incl. BootSector)
    uint8_t  NumFATs;             // 0x10 Usually 2
    uint16_t RootEntCnt;          // 0x11 FAT12/16 only (0 for FAT32)
    uint16_t TotSec16;            // 0x13 Total sectors (FAT12/16)
    uint8_t  Media;               // 0x15 Media descriptor
    uint16_t FATSz16;             // 0x16 FAT12/16 size (0 for FAT32)
    uint16_t SecPerTrk;           // 0x18 CHS
    uint16_t NumHeads;            // 0x1A CHS
    uint32_t HiddSec;             // 0x1C Hidden sectors before partition
    uint32_t TotSec32;            // 0x20 Total sectors FAT32

    // ---- FAT32 Extended BPB ----
    uint32_t FATSz32;             // 0x24 FAT size in sectors
    uint16_t ExtFlags;            // 0x28 Flags
    uint16_t FSVer;               // 0x2A FAT version
    uint32_t RootClus;            // 0x2C First cluster of root directory
    uint16_t FSInfo;              // 0x30 Sector number of FSINFO
    uint16_t BkBootSec;           // 0x32 Sector number of backup boot
    uint8_t  Reserved[12];        // 0x34 Reserved

    uint8_t  DrvNum;              // 0x40 Drive number
    uint8_t  Reserved1;           // 0x41 NT flags
    uint8_t  BootSig;             // 0x42 Extended Signature == 0x29
    uint32_t VolID;               // 0x43 Volume serial number
    char     VolLab[11];          // 0x47 Volume label
    char     FilSysType[8];       // 0x52 "FAT32   "

} FAT32_BPB_t;
#pragma pack(pop)

void printFAT_vol(FAT32_Volume_t* vol) {
    printf(GREEN("FAT32 mounted:\n"));
    printf("  part LBA start      = %lu\n", vol->part_lba_start);
    printf("  bytes/sector        = %u\n", vol->bytes_per_sector);
    printf("  sectors/cluster     = %u\n", vol->sectors_per_cluster);
    printf("  reserved sectors    = %u\n", vol->reserved_sectors);
    printf("  num FATs            = %u\n", vol->num_fats);
    printf("  FAT size (sectors)  = %lu\n", vol->fat_size_sectors);
    printf("  first data sector   = %lu (rel)\n", vol->first_data_sector);
    printf("  root cluster        = %lu\n", vol->root_cluster);
}

int FAT32_Mount(FAT32_Volume_t* vol) {
    if (!vol) return -1;

    uint32_t boot_lba = g_part1_lba_start;
    uint8_t sector[512];
    if (SD_ReadBlock(boot_lba, sector) != HAL_OK) { printf(RED("FAT32_Mount: cannot read boot sector at LBA %lu\n"), boot_lba); return -1; }

    FAT32_BPB_t* bpb = (FAT32_BPB_t*)sector;
    if (bpb->BytsPerSec != 512u) { printf(RED("FAT32_Mount: unsupported BytsPerSec=%u\n"), bpb->BytsPerSec); return -1; }
    if (bpb->SecPerClus == 0u) { printf(RED("FAT32_Mount: SecPerClus=0 is invalid\n")); return -1; }
    if (bpb->RootEntCnt != 0u) { printf(RED("FAT32_Mount: RootEntCnt=%u (expected 0 for FAT32)\n"), bpb->RootEntCnt); return -1;    }
    if (bpb->FATSz32 == 0u) { printf(RED("FAT32_Mount: FATSz32=0 (not a valid FAT32 volume)\n")); return -1; }

    uint32_t first_data_sector =
        (uint32_t)bpb->RsvdSecCnt +
        (uint32_t)bpb->NumFATs * (uint32_t)bpb->FATSz32;

    *vol = (FAT32_Volume_t) {
        .part_lba_start      = boot_lba,
        .bytes_per_sector    = bpb->BytsPerSec,
        .sectors_per_cluster = bpb->SecPerClus,
        .reserved_sectors    = bpb->RsvdSecCnt,
        .num_fats            = bpb->NumFATs,
        .fat_size_sectors    = bpb->FATSz32,
        .root_cluster        = bpb->RootClus,
        .first_data_sector   = first_data_sector,
        .fat_start_lba       = boot_lba + bpb->RsvdSecCnt,
    };

    return 0;
}

FAT32_Volume_t g_fat32;

static uint32_t FAT32_GetNextCluster(FAT32_Volume_t* vol, uint32_t cluster) {
    // printf("FAT32_GetNextCluster\n");
    uint32_t fat_offset = cluster * 4u; // 4 bytes per FAT32 entry
    uint32_t fat_sector_idx = fat_offset / vol->bytes_per_sector;
    uint32_t ent_offset = fat_offset % vol->bytes_per_sector;

    uint8_t sector[512];
    uint32_t fat_lba = vol->fat_start_lba + fat_sector_idx;

    if (SD_ReadBlock(fat_lba, sector) != HAL_OK) {
        printf(RED("FAT32_GetNextCluster: read error at LBA %lu\n"),
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
    // printf("FAT32_ReadCluster\n");

    uint32_t first_sector_of_cluster =
        vol->first_data_sector + (cluster - 2u) * (uint32_t)vol->sectors_per_cluster;

    for (uint32_t i = 0; i < vol->sectors_per_cluster; ++i) {
        uint32_t lba = vol->part_lba_start + first_sector_of_cluster + i;
        if (SD_ReadBlock(lba, buf + i * vol->bytes_per_sector) != HAL_OK) {
            printf(RED("FAT32_ReadCluster: read error at LBA %lu\n"), lba);
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

char fat32_up(char c) {
    if ((c >= 'a') && (c <= 'z')) {
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
    return ((*a == '\0') && (*b == '\0'));
}

static void FAT32_MakeShortName(cStringRO e, cString out, uint32_t out_size) {
    // e[0..7] - name, e[8..10] - ext
    char name[9];
    char ext[4];

    for (int i = 0; i < 8; ++i)
        name[i] = (char)e[i];
    name[8] = '\0';

    for (int i = 0; i < 3; ++i)
        ext[i] = (char)e[8 + i];
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

    if (ext[0] != '\0') snprintf(out, out_size, "%s.%s", name, ext);
    else                snprintf(out, out_size, "%s", name);

}
// one cluster buffer (max 8 KB if SecPerClus up to 16)
static uint8_t cluster_buf[512 * 16];

static void FAT32_ListDir(FAT32_Volume_t* vol, uint32_t start_cluster, int depth) {
    if (start_cluster < 2u)     return;

    uint32_t cluster = start_cluster;

    while (!FAT32_IsEOC(cluster)) {
        if (FAT32_ReadCluster(vol, cluster, cluster_buf) != 0) {
            return;
        }

        uint32_t bytes_in_cluster =
            (uint32_t)vol->bytes_per_sector * vol->sectors_per_cluster;

        for (uint32_t off = 0; (off + 32u) <= bytes_in_cluster; off += 32u) {
            cStringRO e = (cStringRO)&cluster_buf[off];

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
    if (FAT32_Mount(&g_fat32) != 0) { printf(RED("FAT32_PrintTree: mount failed\n")); return; }

    printf("=== FAT32 directory tree ===\n");
    FAT32_ListDir(&g_fat32, g_fat32.root_cluster, 0);
    printf("=== end of tree ===\n");
}


static int FAT32_FindInDir(
    FAT32_Volume_t* vol,
    uint32_t            dir_cluster,
    cStringRO           name,
    uint8_t             want_dir,
    FAT32_DirEntryInfo* out) {

    if (dir_cluster < 2u) return -1;

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

int FAT32_FileOpen(cStringRO path, FAT32_File_t* fh) {
    FAT32_DirEntryInfo info;
    if (!fh) return -1;
    if (FAT32_FindPath(path, &info) != 0) { printf("FAT32_FileOpen: file '%s' not found\n", path); return -1; }

    FAT32_Volume_t* vol = &g_fat32;
    *fh = (FAT32_File_t) {
        .vol              = vol,
        .first_cluster    = info.cluster,
        .current_cluster  = info.cluster,
        .file_size        = info.size,
        .position         = 0,
        .bytes_per_cluster= (uint32_t)vol->bytes_per_sector * vol->sectors_per_cluster,
        .cluster_index    = 0,
        .cluster_valid    = 0,
        // cluster_buf НЕ трогаем — он просто остаётся как есть
    };

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

int FAT32_FileRead(FAT32_File_t* fh, void* dst, uint32_t bytes_to_read, uint32_p out_read) {
    if (!fh || !dst) return -1;

    uint8_t* out = (uint8_t*)dst;

    if (fh->position >= fh->file_size) {
        if (out_read) *out_read = 0;
        return 0; // EOF
    }

    uint32_t remaining = fh->file_size - fh->position;
    if (bytes_to_read > remaining)
        bytes_to_read = remaining;


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
            fh->current_cluster = next;
            fh->cluster_valid = 0;
            if (FAT32_IsEOC(next)) {
                break;
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

int FAT32_FileSeekSet(FAT32_File_t* fh, uint32_t new_pos) {
    if (!fh) return -1;

    // clamp to EOF
    if (new_pos > fh->file_size) {
        new_pos = fh->file_size;
    }

    // быстрый путь: в самое начало
    if (new_pos == 0u) {
        fh->current_cluster = fh->first_cluster;
        fh->position = 0u;
        fh->cluster_index = 0u;
        fh->cluster_valid = 0u;
        return 0;
    }

    // быстрый путь: seek ровно в конец файла — читать там всё равно никто не будет
    if (new_pos == fh->file_size) {
        fh->position = new_pos;
        fh->cluster_valid = 0u;
        // current_cluster/cluster_index можно не трогать: Read вернёт EOF по position
        return 0;
    }

    uint32_t cluster_size = fh->bytes_per_cluster;
    if (cluster_size == 0u) {
        return -1;
    }

    // какой по счёту кластер и смещение внутри него
    uint32_t target_cluster_index = new_pos / cluster_size;
    // uint32_t inside_cluster       = new_pos % cluster_size; // для логики не нужен

    uint32_t clus = fh->first_cluster;
    uint32_t idx = 0u;

    // идём по FAT-цепочке, НЕ читая данных файла
    while (idx < target_cluster_index) {
        uint32_t next = FAT32_GetNextCluster(fh->vol, clus);

        if (FAT32_IsEOC(next) || next < 2u) {
            // цепочка закончилась раньше, чем ожидали — фиксируемся на EOF
            fh->position = fh->file_size;
            fh->current_cluster = next;
            fh->cluster_index = idx; // фактическая длина цепочки
            fh->cluster_valid = 0u;
            return 0;
        }

        clus = next;
        idx++;
    }

    // теперь clus — это кластер, содержащий new_pos
    fh->current_cluster = clus;
    fh->position = new_pos;
    fh->cluster_index = idx;
    fh->cluster_valid = 0u;  // буфер нужно будет перезагрузить при следующем чтении

    return 0;
}