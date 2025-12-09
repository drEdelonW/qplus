
#include <stdio.h>
#include "types.h"

void FAT32_PrintIndent(int depth) {
    for (int i = 0; i < depth; ++i)
        printf("  ");
}


char fat32_up(char c) {
    if ((c >= 'a') && (c <= 'z'))
        return (char)(c - 'a' + 'A');
    else
        return c;
}

int FAT32_NameEquals(cStringRO a, cStringRO b) {
    while (*a && *b) {
        char ca = fat32_up(*a);
        char cb = fat32_up(*b);
        if (ca != cb) return 0;
        ++a;
        ++b;
    }
    return ((*a == '\0') && (*b == '\0'));
}


int FAT32_IsEOC(uint32_t cl) {
    return (cl >= 0x0FFFFFF8u);
}


void FAT32_MakeShortName(cStringRO e, cString out, uint32_t out_size) {
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