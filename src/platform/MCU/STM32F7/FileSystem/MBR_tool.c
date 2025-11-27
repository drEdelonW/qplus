#include "MBR.h"
#include <stdio.h>
#include "types.h"
#include "SD_TF.h"
#include "fs_FAT32.h"



static cStringRO MBR_PartTypeStr(uint8_t type) {
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


void SD_PrintMBR() {
    uint8_t buf[512];
        printf("SD_PrintMBR: begin\n");

    if (SD_ReadBlock(0, buf) != HAL_OK) {
        printf("SD_PrintMBR: can't read LBA0\n");
        return;
    }

    uint8_t sig_lo = buf[510];
    uint8_t sig_hi = buf[511];
    if ((sig_lo != 0x55) || (sig_hi != 0xAA)) {
        printf("SD_PrintMBR: no valid MBR signature (0x%02X 0x%02X)\n",
            (uint16_t)sig_lo, (uint16_t)sig_hi);
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

        if ((type == 0) && (lba_first == 0) && (sectors == 0)) {
            continue; // empty entry
        }

        uint64_t bytes = (uint64_t)sectors * 512u;
        uint32_t mb = (uint32_t)(bytes / (1024u * 1024u));

        printf("Part %lu:\n", (uint32_t)(i + 1u));
        printf("  Boot      : %s (0x%02lX)\n",
            (status == 0x80) ? "yes" : "no",
            (uint32_t)status);
        printf("  Type      : 0x%02X (%s)\n", type, MBR_PartTypeStr(type));
        printf("  LBA start : %lu\n", lba_first);
        printf("  Sectors   : %lu\n", sectors);
        printf("  Size      : %lu MB (approx)\n", mb);

        if ((g_part1_lba_start == 0) &&
            (
                (type == 0x0B) ||
                (type == 0x0C))
            ) {
            g_part1_lba_start = lba_first;
        }
    }

    printf("===========================\n");
    printf("FAT32 partition LBA start = %lu\n", (unsigned long)g_part1_lba_start);
}