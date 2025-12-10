#include "fs_FAT32.h"
#include "types.h"
#include "terminal_tools.h"
#include <string.h>

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

int FAT32_FileLoadCluster(FAT32_File_t* fh) {
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

#if 0
int FAT32_FileRewind(FAT32_File_t* fh) {
    if (!fh) return -1;

    fh->current_cluster = fh->first_cluster;
    fh->position = 0;
    fh->cluster_index = 0;
    fh->cluster_valid = 0;

    return 0;
}
#endif
int FAT32_FileSeekSet(FAT32_File_t* fh, uint32_t new_pos) {
    if (!fh) return -1;

    // clamp to EOF
    if (new_pos > fh->file_size) {
        new_pos = fh->file_size;
    }
    else if (new_pos == 0u) {
        fh->current_cluster = fh->first_cluster;
        fh->position        = 0u;
        fh->cluster_index   = 0u;
        fh->cluster_valid   = 0u;
        return 0;
    }
    else if (new_pos == fh->file_size) { // быстрый путь: seek ровно в конец файла — читать там всё равно никто не будет
        fh->position        = new_pos;
        fh->cluster_valid   = 0u;
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

        if (FAT32_IsEOC(next) || (next < 2u)) {
            // цепочка закончилась раньше, чем ожидали — фиксируемся на EOF
            fh->position        = fh->file_size;
            fh->current_cluster = next;
            fh->cluster_index   = idx; // фактическая длина цепочки
            fh->cluster_valid   = 0u;
            return 0;
        }

        clus = next;
        idx++;
    }

    // теперь clus — это кластер, содержащий new_pos
    fh->current_cluster = clus;
    fh->position        = new_pos;
    fh->cluster_index   = idx;
    fh->cluster_valid   = 0u;  // буфер нужно будет перезагрузить при следующем чтении

    return 0;
}

static void SDFS_MakeFatPath(cStringRO quakePath, char* out, uint32_t outSize) {
    cStringRO base = "QUAKE/";
    uint32_t i = 0;

    // копируем префикс
    while (*base && ((i + 1) < outSize)) {
        out[i++] = *base++;
    }

    // копируем исходный путь, поднимая регистр и заменяя '\' на '/'
    for (cStringRO p = quakePath; (*p && ((i + 1) < outSize)); ++p) {
        char c = *p;
        if (c == '\\')  c = '/';
        // uppercase
        c = fat32_up(c);
        out[i++] = c;
    }

    out[i] = '\0';
}

#define SDFS_MAX_OPEN_FILES 9

typedef struct {
    bool          used;
    FAT32_File_t  file;
} SD_FileSlot;

static SD_FileSlot s_sdFiles[SDFS_MAX_OPEN_FILES] = { 0 };

static int SDFS_AllocHandle(void) {
    for (int i = 0; i < SDFS_MAX_OPEN_FILES; ++i) {
        if (!s_sdFiles[i].used) {
            s_sdFiles[i].used = true;
            // printf("SDFS_AllocHandle USED SLOTS [%i]\n", i);
            return i;
        }
    }
    printf("SDFS_AllocHandle NO MORE FREE SLOTS!\n");
    return -1;
}

static SD_FileSlot* SDFS_GetSlot(int hnd) {
    if ((hnd < 0) ||
        (hnd >= SDFS_MAX_OPEN_FILES) ||
        (!s_sdFiles[hnd].used)
        )   return NULL;

    return &s_sdFiles[hnd];
}


int _open(char* path, int flags, ...) {
    (void)path;
    (void)flags;
    int fh = SDFS_AllocHandle();
    if (fh == -1) return -1;
    char fatPath[128];
    SDFS_MakeFatPath(path, fatPath, sizeof(fatPath));
    if (FAT32_FileOpen(fatPath, &SDFS_GetSlot(fh)->file) != -1)
        return fh;

    // printf(RED("_open(path:[%s] flags:0x%X);\n"), path, flags);
    SDFS_GetSlot(fh)->used = false;
    return -1;
}


int _lseek(int file, int ptr, int dir) {
    // printf(RED("_lseek(file:%i, ptr:%i, dir:%i);\n"), file, ptr, dir);
    if ((dir == SEEK_SET) &&
        (!FAT32_FileSeekSet(&SDFS_GetSlot(file)->file, ptr))
    )
        return ptr;

    printf(RED("_lseek(file:%i, ptr:%i, dir:%i);\n"), file, ptr, dir);
    return 0;
}


int _read(int file, char* ptr, int len) {
    // printf(RED("_read(file:%i, ptr:%p, len:%i);\n"), file, ptr, len);

    return Sys_FileRead(file, ptr, len);
}
int _close(int file) {
    printf(RED("_close(file:%i);\n"), file);
    SDFS_GetSlot(file)->used = false;
    return -1;
}
/*=======================[SysFS part begin]=======================*/
int Sys_FileOpenRead(cStringRO quakePath, int* hnd) {
    if (!hnd) return -1;

    if (SD_FS_Init() != 0) {
        *hnd = -1;
        return -1;
    }

    int slot = SDFS_AllocHandle();
    if (slot < 0) {
        *hnd = -1;
        return -1;
    }

    char fatPath[128];
    SDFS_MakeFatPath(quakePath, fatPath, sizeof(fatPath));

    FAT32_File_t* f = &s_sdFiles[slot].file;

    // обнулим, если вдруг FAT32_FileOpen рассчитывает на чистое состояние
    memset(f, 0, sizeof(FAT32_File_t));

    if (FAT32_FileOpen(fatPath, f) != 0) {
        // открыть не удалось — слот освобождаем
        s_sdFiles[slot].used = false;
        *hnd = -1;
        return -1;
    }

    *hnd = slot;
    return (int)f->file_size;
}
void Sys_FileClose(int handle) {
    SD_FileSlot* slot = SDFS_GetSlot(handle);
    if (!slot) return;

    FAT32_FileClose(&slot->file);
    slot->used = false;
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

int Sys_FileSeek(int handle, int position) {
    SD_FileSlot* slot = SDFS_GetSlot(handle);
    if (!slot) return -1;

    if (FAT32_FileSeekSet(&slot->file, (uint32_t)position) != 0) {
        return -1;
    }
    return 0;
}