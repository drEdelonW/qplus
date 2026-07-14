#include "GlobVars.h"
#include "endian_tools.h"

globalvars_p   pr_global_struct;   // much more
float_p        pr_globals;         // same as pr_global_struct


void initProgGlobals(progLump_t pl) {
    pr_global_struct = GetPtrFromLump(pl);
    pr_globals = GetPtrFromLump(pl);
    for (int i = 0; i < pl.num; i++)
        pr_globals[i] = LittleFloat(pr_globals[i]);
}


void RETURN_EDICT(edict_p edict) { ((int32_p)pr_globals)[OFS_RETURN] = ED_GetEDictOffs(edict); }
float_p GV_pFloat(PrOfs_e Param) { return (float_p)&pr_globals[Param]; }
int32_p GV_pInt(PrOfs_e Param) { return (int32_p)&pr_globals[Param]; }
vec3_p GV_pVec3(PrOfs_e Param) { return (vec3_p)&pr_globals[Param]; }
ang3_p GV_pAng3(PrOfs_e Param) { return (ang3_p)&pr_globals[Param]; }
string_p GV_pqStr(PrOfs_e Param) { return (string_p)&pr_globals[Param]; }

