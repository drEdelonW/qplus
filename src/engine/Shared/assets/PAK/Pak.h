#pragma once

#include "platformdefs.h"
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
    char    name[MAX_QPATH];
    uint32_t filepos;
    uint32_t filelen;
} packfile_t;
typedef packfile_t* packfile_p;

typedef struct pack_s {
    char        filename[MAX_OSPATH];
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