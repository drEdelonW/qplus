#include "fs_FAT32.h"
#include "types.h"
#include "terminal_tools.h"

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