#pragma once

#include "progLump.h"
#include "types.h"
#include "assert.h"
#include "pr_qString.h"
#include <stdio.h>

// dDef: runtime descriptor of a QuakeC variable (name, type, offset).
// Used as reflection over VM memory (progs.dat layout).
//
// Globals: variables stored in pr_globals[] (VM-wide state).
// Fields:  per-entity variables stored inside edict payload.
//
// Access pattern: name -> dDef -> offset -> raw memory read/write.
// #include "vmValue.h"

typedef struct {
    uint16_t    type;   // [etype_t] if DEF_SAVEGLOBAL bit is set
                        // the variable needs to be saved in savegames
    uint16_t    ofs;
    string_t    s_name;
} dDef_t;
typedef dDef_t* dDef_p;
STATIC_ASSERT_SIZE(dDef_t, 2*2 + 4);    // 8

extern dDef_p   pr_fielddefs;

#ifdef __cplusplus
extern "C" {
#endif

    void ED_InitCache();
    void initProgDefs(TypeLess_ptr base, progLump_t plg, progLump_t plf);

    void ED_WriteGlobals(FILE* f);
    void ED_ParseGlobals(cString data);
    dDef_p ED_GlobalAtOfs(int ofs);
    dDef_p ED_FindGlobal(cString name);

    dDef_p ED_FieldAtOfs(int ofs);
    dDef_p ED_FindField(cString name);
    dDef_p ED_FindFieldCached(cString name);

#ifdef __cplusplus
}
#endif