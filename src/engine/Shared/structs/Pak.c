#include "Pak.h"
#include "enginedefs.h"
#include "zone.h"
#include "sys.h"
#include "endian_tools.h"
#include "crc.h"
#include "console.h"
#include <string.h>

bool     contModified;   // set true if using non-id files

#define PAK0_COUNT              339
#define PAK0_CRC                32981

/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
pack_p COM_LoadPackFile(cStringRO packfile) {

    int packhandle;
    if (Sys_FileOpenRead(packfile, &packhandle) == -1) {
        //              Con_Printf ("Couldn't open %s\n", packfile);
        return NULL;
    }
    dpackHeader_t header;
    Sys_FileRead(packhandle, (TypeLess_ptr)&header, sizeof(header));
    if ((header.id[0] != 'P') ||
        (header.id[1] != 'A') ||
        (header.id[2] != 'C') ||
        (header.id[3] != 'K'))
        Sys_Error("%s is not a packfile", packfile);
    header.dirofs = LittleLong(header.dirofs);
    header.dirlen = LittleLong(header.dirlen);

    int numpackfiles = header.dirlen / sizeof(dpackfile_t);

    if (numpackfiles > MAX_FILES_IN_PACK)
        Sys_Error("%s has %i files", packfile, numpackfiles);

    if (numpackfiles != PAK0_COUNT)
        contModified = true;    // not the original file

    packfile_p newfiles = Hunk_AllocName(numpackfiles * sizeof(packfile_t), "packfile");

    Sys_FileSeek(packhandle, header.dirofs);
    dpackfile_t info[MAX_FILES_IN_PACK];
    Sys_FileRead(packhandle, (TypeLess_ptr)info, header.dirlen);

    // crc the directory to check for modifications
    uint16_t crc;
    CRC_Init(&crc);
    for (int i = 0; i < header.dirlen; i++)
        CRC_ProcessByte(&crc, ((uint8_p)info)[i]);

    if (crc != PAK0_CRC)
        contModified = true;

    // parse the directory
    for (int i = 0; i < numpackfiles; i++) {
        strcpy(newfiles[i].name, info[i].name);
        newfiles[i].filepos = LittleLong(info[i].filepos);
        newfiles[i].filelen = LittleLong(info[i].filelen);
    }

    pack_p pack = Hunk_Alloc(sizeof(pack_t));
    strcpy(pack->filename, packfile);
    pack->handle = packhandle;
    pack->numfiles = numpackfiles;
    pack->files = newfiles;

    Con_Printf("Added packfile %s (%i files)\n", packfile, numpackfiles);
    return pack;
}
