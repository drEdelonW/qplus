#pragma once
#include "Edict.h"
#include <stdio.h>
#include "server.h"

extern uint32_t     pr_edict_size;  // in bytes

#ifdef __cplusplus
extern "C" {
#endif
    void ED_Init();

    edict_p ED_Alloc();
    void ED_Free(edict_p ed);

    cString ED_NewString(cString string);   // returns a copy of the string allocated from the server's string heap

    dDef_p ED_FieldAtOfs(int ofs);

    void ED_Print(edict_p ed);
    void ED_Write(FILE* f, edict_p ed);
    cString ED_ParseEdict(cString data, edict_p ent);

    void ED_WriteGlobals(FILE* f);
    void ED_ParseGlobals(cString data);

    void ED_LoadFromFile(cString data);
    bool ED_ParseEpair(TypeLess_ptr base, dDef_p key, cString s);

    edict_p EDICT_NUM(uint32_t n);
    uint32_t NUM_FOR_EDICT(edict_p e);

#   define NEXT_EDICT(e)        ((edict_p)((uint8_p)(e) + pr_edict_size))
#   define PROG_EDICT_BASE      ((uint8_p)sv.edicts)
#   define PROG_TO_EDICT(ofs)   ((edict_p)(PROG_EDICT_BASE + (uint32_t)(ofs)))
#   define EDICT_TO_PROG(e)     ((int32_t)((uint8_p)(e) - PROG_EDICT_BASE))
#   define G_EDICT(o)       PROG_TO_EDICT((uint32_t)G_INT((o)))
#   define G_EDICTNUM(o)    NUM_FOR_EDICT(G_EDICT((o)))

    void ED_PrintEdicts();
    void ED_PrintNum(uint32_t ent);

    eval_p GetEdictFieldValue(edict_p ed, cString field);
#ifdef __cplusplus
}
#endif