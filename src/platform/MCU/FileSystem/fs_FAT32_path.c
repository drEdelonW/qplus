#include "fs_FAT32.h"
#include "SD_TF.h"
#include "terminal_tools.h"


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
