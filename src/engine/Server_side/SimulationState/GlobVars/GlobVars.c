#include "GlobVars.h"
#include "endian_tools.h"

static globalvars_p   pr_global_struct;   // much more
static float_p  _prGlobals;        // same as pr_global_struct
void initProgGlobals(progLump_t pl) {
    pr_global_struct = GetPtrFromLump(pl);
    _prGlobals = GetPtrFromLump(pl);
    for (int i = 0; i < pl.num; i++)
        _prGlobals[i] = LittleFloat(_prGlobals[i]);
}

globalvars_p GV_pGame() { return pr_global_struct; }

// TypeLessPtr GV_pBaseGlobals() { return _prGlobals; }
void RETURN_EDICT(edict_p edict) { ((int32_p)_prGlobals)[OFS_RETURN] = ED_GetEDictOffs(edict); }
edict_p GV_pEdict(PrOfs_e Param) { return (edict_p)&_prGlobals[Param]; }
eval_p GV_pEval(PrOfs_e Param) { return (eval_p)&_prGlobals[Param]; }
float_p GV_pFloat(PrOfs_e Param) { return (float_p)&_prGlobals[Param]; }
int32_p GV_pInt(PrOfs_e Param) { return (int32_p)&_prGlobals[Param]; }
vec3_p GV_pVec3(PrOfs_e Param) { return (vec3_p)&_prGlobals[Param]; }
ang3_p GV_pAng3(PrOfs_e Param) { return (ang3_p)&_prGlobals[Param]; }
string_p GV_pqStr(PrOfs_e Param) { return (string_p)&_prGlobals[Param]; }

