#pragma once


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