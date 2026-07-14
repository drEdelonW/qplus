#pragma once

#include "enginedefs.h"
#include "types.h"
/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

extern bool contModified;   // set true if using non-id files

#include "assert.h"

//
// on disk
//
typedef struct {
    char    name[56];
    uint32_t filepos;
    uint32_t filelen;
} dpackfile_t;      STATIC_ASSERT_SIZE(dpackfile_t, 56 + 4 + 4); // 64

typedef struct {
    char    id[4];
    uint32_t dirofs;
    uint32_t dirlen;
} dpackHeader_t;    STATIC_ASSERT_SIZE(dpackHeader_t, 4 + 4 + 4); // 12

//
// in memory
//
typedef struct {
    qPathStr_t     name;
    uint32_t    filepos;
    uint32_t    filelen;
} packfile_t;
typedef packfile_t* packfile_p;

typedef struct pack_s {
    fsPathStr_t filename;
    uint32_t    handle;
    uint32_t    numfiles;
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