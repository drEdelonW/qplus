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

#   define G_EDICT(o)           ED_GetEDictByOffs((uint32_t)G_INT((o)))
#   define G_EDICTNUM(o)        ED_GetEDictIdx(G_EDICT((o)))
#   define RETURN_EDICT(edict)  (((int *)pr_globals)[OFS_RETURN] = ED_GetEDictOffs(edict))

    edict_p ED_GetEDictByIdx(uint32_t idx);
    uint32_t ED_GetEDictIdx(edict_p e);

    edict_p ED_GetEDictByOffs(int32_t eIdx);
    int32_t ED_GetEDictOffs(edict_p ePtr);

    // edict_p ED_GetEDictFirst();
    edict_p ED_GetEDictNext(edict_p edict);

    void ED_PrintEdicts();
    void ED_PrintNum(uint32_t ent);

    eval_p GetEdictFieldValue(edict_p ed, cString field);

    edict_p FindViewthing();
#ifdef __cplusplus
}
#endif