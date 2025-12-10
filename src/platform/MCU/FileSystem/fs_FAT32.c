#include "MBR.h"
#include "SD_TF.h"
#include "fs_FAT32.h"
#include "terminal_tools.h"


// one cluster buffer (max 8 KB if SecPerClus up to 16)
uint8_t cluster_buf[512 * 16];

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
    if (bpb->BytsPerSec != 512u) {  printf(RED("FAT32_Mount: unsupported BytsPerSec=%u\n"), bpb->BytsPerSec); return -1; }
    if (bpb->SecPerClus == 0u) {    printf(RED("FAT32_Mount: SecPerClus=0 is invalid\n")); return -1; }
    if (bpb->RootEntCnt != 0u) {    printf(RED("FAT32_Mount: RootEntCnt=%u (expected 0 for FAT32)\n"), bpb->RootEntCnt); return -1;    }
    if (bpb->FATSz32 == 0u) {       printf(RED("FAT32_Mount: FATSz32=0 (not a valid FAT32 volume)\n")); return -1; }

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

uint32_t FAT32_GetNextCluster(FAT32_Volume_t* vol, uint32_t cluster) {
    uint32_t fat_offset = cluster * 4u; // 4 bytes per FAT32 entry
    uint32_t fat_sector_idx = fat_offset / vol->bytes_per_sector;
    uint32_t ent_offset = fat_offset % vol->bytes_per_sector;

    uint8_t sector[512];
    uint32_t fat_lba = vol->fat_start_lba + fat_sector_idx;

    if (SD_ReadBlock(fat_lba, sector) != HAL_OK) {
        printf(RED("FAT32_GetNextCluster: read error at LBA %lu\n"), fat_lba);
        return 0x0FFFFFFF; // treat as end
    }

    uint32_t val = rd32_le(&sector[ent_offset]);
    val &= 0x0FFFFFFF; // FAT32 uses low 28 bits

    return val;
}

int FAT32_ReadCluster(FAT32_Volume_t* vol, uint32_t cluster, uint8_p buf) {
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


