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

extern uint8_t cluster_buf[];


char fat32_up(char c);
int FAT32_Mount(FAT32_Volume_t* vol);
int FAT32_FileOpen(cStringRO path, FAT32_File_t* fh);
void FAT32_FileClose(FAT32_File_t* fh);
int FAT32_FileRead(FAT32_File_t* fh, void* dst, uint32_t bytes_to_read, uint32_p out_read);
int FAT32_FileSeekSet(FAT32_File_t* fh, uint32_t new_pos);


int FAT32_IsEOC(uint32_t cl);
int FAT32_FindPath(cStringRO path, FAT32_DirEntryInfo* out_file);
