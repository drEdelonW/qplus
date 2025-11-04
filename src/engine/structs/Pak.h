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
    int32_t filepos, filelen;
} packfile_t;
typedef packfile_t* packfile_p;

typedef struct pack_s {
    char        filename[MAX_OSPATH];
    int32_t     handle;
    int32_t     numfiles;
    packfile_p  files;
} pack_t;
typedef pack_t* pack_p;

//
// on disk
//
typedef struct {
    char    name[56];
    int32_t filepos, filelen;
} dpackfile_t;

typedef struct {
    char    id[4];
    int32_t dirofs;
    int32_t dirlen;
} dpackHeader_t;

#define MAX_FILES_IN_PACK       2048


pack_p COM_LoadPackFile(cStringRO packfile);
