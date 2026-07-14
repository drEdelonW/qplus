#pragma once

#include "progdefs.h"
#include "Edict.h"  // edict_p
#include "vmValue.h"  // eval_p

typedef globalvars_t* globalvars_p;
extern globalvars_p pr_global_struct;   // global variable of game settings
// extern float_p      pr_globals;         // same as pr_global_struct
// float_p GV_pBaseGlobals();
#if 0
// #define RETURN_EDICT(edict) (((int *)pr_globals)[OFS_RETURN] = ED_GetEDictOffs(edict))
// #define G_FLOAT(o)          (pr_globals[(o)])
// #define G_INT_P(o)          ((int32_p)pr_globals)[(o)]
// #define G_INT(o)            (*(int32_p)&pr_globals[(o)])
// #define G_VECTOR(o)         (*(vec3_p)(&pr_globals[(o)]))
// #define G_ANGLES(o)         (*(ang3_p)(&pr_globals[(o)]))
// #define G_STRING(o)         PR_GetQString(*(qVmString_t*)&pr_globals[(o)])

// #define PR_Freturn          G_FLOAT(OFS_RETURN) =
// #define PR_FRETURN(ret)      G_FLOAT(OFS_RETURN) = (ret)
// #define PR_Ireturn           G_INT(OFS_RETURN) =
// #define PR_IRETURN(ret)      G_INT(OFS_RETURN) = (ret)

#else

void RETURN_EDICT(edict_p edict);
edict_p GV_pEdict(PrOfs_e Param);
#define G_EDICT(o)  (*(GV_pEdict(o)))
eval_p GV_pEval(PrOfs_e Param);
#define G_EVAL(o)  (*(GV_pEval(o)))
float_p GV_pFloat(PrOfs_e Param);
#define G_FLOAT(o)  (*(GV_pFloat(o)))
int32_p GV_pInt(PrOfs_e Param);
#define G_INT_P(o)  (GV_pInt(o))
#define G_INT(o)    (*(GV_pInt(o)))
vec3_p GV_pVec3(PrOfs_e Param);
#define G_VECTOR(o)    (*(GV_pVec3(o)))
ang3_p GV_pAng3(PrOfs_e Param);
#define G_ANGLES(o)    (*(GV_pAng3(o)))
string_p GV_pqStr(PrOfs_e Param);
#define G_STRING(o)    PR_GetQString((*GV_pqStr(o)))
#define PR_Freturn      G_FLOAT(OFS_RETURN) =

#endif

#ifdef __cplusplus
extern "C" {
#endif

    void initProgGlobals(progLump_t pl);

#ifdef __cplusplus
}
#endif