#pragma once

#include "types.h"
#include "assert.h"
#include "pr_qString.h"

typedef struct {
    uint16_t    type;   // [etype_t] if DEF_SAVEGLOBAL bit is set
                        // the variable needs to be saved in savegames
    uint16_t    ofs;
    string_t    s_name;
} dDef_t;
typedef dDef_t* dDef_p;
STATIC_ASSERT_SIZE(dDef_t, 2*2 + 4);    // 8


#define MAX_FIELD_LEN (64)
#define GEFV_CACHESIZE (2)

typedef struct {
    dDef_p  pcache;
    char    field[MAX_FIELD_LEN];
} gefv_cache;

extern gefv_cache   gefvCache[GEFV_CACHESIZE];

extern dDef_p       pr_globaldefs;
extern dDef_p       pr_fielddefs;

#ifdef __cplusplus
extern "C" {
#endif

    dDef_p ED_GlobalAtOfs(int ofs);

#ifdef __cplusplus
}
#endif