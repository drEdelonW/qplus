#pragma once

#include "enginedefs.h"
#include "types.h"
/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

extern bool contModified;   // set true if using non-id files

//
// in memory
//
typedef struct {
    qPath_t     name;
    uint32_t    filepos;
    uint32_t    filelen;
} packfile_t;
typedef packfile_t* packfile_p;

typedef struct pack_s {
    fsPath_t    filename;
    uint32_t     handle;
    uint32_t     numfiles;
    packfile_p  files;
} pack_t;
typedef pack_t* pack_p;

#ifdef __cplusplus
extern "C" {
#endif

    pack_p COM_LoadPackFile(cStringRO packfile);

#ifdef __cplusplus
}
#endif