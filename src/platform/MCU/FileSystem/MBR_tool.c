#include "MBR.h"
#include <stdio.h>
#include "types.h"
#include "SD_TF.h"

// global FAT32 partition LBA (first FAT32 partition found)
uint32_t g_part1_lba_start = 0;

typedef struct {
    uint8_t  status;
    uint8_t  type;
    uint32_t lba_first;
    uint32_t sectors;
} MBR_PartEntry_t;

static MBR_PartEntry_t g_mbr_parts[4];
static int g_mbr_inited = 0;

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

/* Read MBR from SD, parse partition table and select first FAT32.
 * Fills g_mbr_parts[] and g_part1_lba_start.
 * Returns 1 on success, 0 on error.
 */
int SD_MBR_Init(void) {
    uint8_t buf[512];

    if (SD_ReadBlock(0u, buf) != HAL_OK) {
        printf("SD_MBR_Init: can't read LBA0\n");
        g_mbr_inited = 0;
        return 0;
    }

    const uint8_t sig_lo = buf[510];
    const uint8_t sig_hi = buf[511];

    if ((sig_lo != 0x55u) || (sig_hi != 0xAAu)) {
        printf("SD_MBR_Init: no valid MBR signature (0x%02X 0x%02X)\n",
               (unsigned int)sig_lo, (unsigned int)sig_hi);
        g_mbr_inited = 0;
        return 0;
    }

    g_part1_lba_start = 0u;

    // clear entries
    for (uint32_t i = 0u; i < 4u; ++i) {
        g_mbr_parts[i].status    = 0u;
        g_mbr_parts[i].type      = 0u;
        g_mbr_parts[i].lba_first = 0u;
        g_mbr_parts[i].sectors   = 0u;
    }

    for (uint32_t i = 0u; i < 4u; ++i) {
        const uint32_t off = 0x1BEu + i * 16u;
        cStringRO  e   = (cStringRO)&buf[off];

        MBR_PartEntry_t *p = &g_mbr_parts[i];

        p->status    = e[0];
        p->type      = e[4];
        p->lba_first = rd32_le(e + 8);
        p->sectors   = rd32_le(e + 12);

        if ((p->type == 0u) && (p->lba_first == 0u) && (p->sectors == 0u)) {
            continue; // empty entry
        }

        if ((g_part1_lba_start == 0u) &&
            ((p->type == 0x0Bu) || (p->type == 0x0Cu))) {
            g_part1_lba_start = p->lba_first;
        }
    }

    g_mbr_inited = 1;
    return 1;
}

/* Print parsed MBR partition table and FAT32 LBA. */
void SD_MBR_Print(void) {
    if (!g_mbr_inited) {
        printf("SD_MBR_Print: MBR not initialized\n");
        return;
    }

    printf("=== MBR Partition Table ===\n");

    for (uint32_t i = 0u; i < 4u; ++i) {
        const MBR_PartEntry_t *p = &g_mbr_parts[i];

        if ((p->type == 0u) && (p->lba_first == 0u) && (p->sectors == 0u)) {
            continue; // empty entry
        }

        const uint64_t bytes = (uint64_t)p->sectors * 512u;
        const uint32_t mb    = (uint32_t)(bytes / (1024u * 1024u));

        printf("Part %lu:\n", (i + 1u));
        printf("  Boot      : %s (0x%02X)\n",
               (p->status == 0x80u) ? "yes" : "no",
               p->status);
        printf("  Type      : 0x%02X (%s)\n",
               p->type,
               MBR_PartTypeStr(p->type));
        printf("  LBA start : %lu\n",  p->lba_first);
        printf("  Sectors   : %lu\n",  p->sectors);
        printf("  Size      : %lu MB (approx)\n", mb);
    }

    printf("===========================\n");
    printf("FAT32 partition LBA start = %lu\n",
           g_part1_lba_start);
}

/* Legacy wrapper: keeps old name, now just init + print. */
void SD_PrintMBR(void) {
    if (!SD_MBR_Init()) return;
    SD_MBR_Print();
}