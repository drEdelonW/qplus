#pragma once
// #include <stdint.h>
#include "types.h"

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
    uint32_t cluster;
    uint32_t size;
    uint8_t  attr;
} FAT32_DirEntryInfo;

extern FAT32_Volume_t g_fat32;

typedef struct {
    FAT32_Volume_t* vol;

    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t file_size;
    uint32_t position;           // общее смещение от начала файла

    uint32_t bytes_per_cluster;

    uint32_t cluster_index;      // номер кластера в цепочке (0,1,2,...)
    uint8_t  cluster_valid;      // флаг, что cluster_buf актуален

    uint8_t  cluster_buf[512 * 16]; // один кластер (512 * sectors_per_cluster)
} FAT32_File_t;



char fat32_up(char c);
int FAT32_Mount(FAT32_Volume_t* vol);
int FAT32_FileOpen(cStringRO path, FAT32_File_t* fh);
void FAT32_FileClose(FAT32_File_t* fh);
int FAT32_FileRead(FAT32_File_t* fh, void* dst, uint32_t bytes_to_read, uint32_p out_read);
int FAT32_FileSeekSet(FAT32_File_t* fh, uint32_t new_pos);
