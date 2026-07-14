#include "GlobVars.h"
#include "endian_tools.h"

static globalvars_p   pr_global_struct; // much more
static float_p  _prGlobals;             // same as pr_global_struct

void initProgGlobals(progLump_t pl) {
    pr_global_struct = GetPtrFromLump(pl);
    _prGlobals = GetPtrFromLump(pl);
    for (int i = 0; i < pl.num; i++)
        _prGlobals[i] = LittleFloat(_prGlobals[i]);
}

globalvars_p GV_pGame() {
    return pr_global_struct;
}
float_p GV_pGlob(PrOfs_e Param) {
    return &_prGlobals[Param];
}


edict_p GV_pEdict(PrOfs_e Param) { return (edict_p)GV_pGlob(Param); }
eval_p GV_pEval(PrOfs_e Param) { return (eval_p)GV_pGlob(Param); }
float_p GV_pFloat(PrOfs_e Param) { return (float_p)GV_pGlob(Param); }
int32_p GV_pInt(PrOfs_e Param) { return (int32_p)GV_pGlob(Param); }
vec3_p GV_pVec3(PrOfs_e Param) { return (vec3_p)GV_pGlob(Param); }
ang3_p GV_pAng3(PrOfs_e Param) { return (ang3_p)GV_pGlob(Param); }
string_p GV_pqStr(PrOfs_e Param) { return (string_p)GV_pGlob(Param); }
void RETURN_EDICT(edict_p edict) { *GV_pInt(OFS_RETURN) = ED_GetEDictOffs(edict); }

