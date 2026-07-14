#pragma once

#include "progdefs.h"
#include "Edict.h"  // edict_p
#include "vmValue.h"  // eval_p

#define G_PTR(o)        (GV_pInt(o))
#define G_INT(o)        (*(GV_pInt(o)))
#define G_FLOAT(o)      (*(GV_pFloat(o)))
#define G_STRING(o)     PR_GetQString((*GV_pqStr(o)))
#define G_VECTOR(o)     (*(GV_pVec3(o)))
#define G_ANGLES(o)     (*(GV_pAng3(o)))
#define G_EVAL(o)       (*(GV_pEval(o)))

#define PR_Freturn      G_FLOAT(OFS_RETURN) =

#ifdef __cplusplus
extern "C" {
#endif

    globalvars_p pGame();   // global variable of game settings
    void RETURN_EDICT(edict_p edict);
    edict_p GV_pEdict(PrOfs_e Param);
    // #define G_EDICT(o)      (*(GV_pEdict(o)))
    eval_p GV_pEval(PrOfs_e Param);
    float_p GV_pFloat(PrOfs_e Param);
    int32_p GV_pInt(PrOfs_e Param);
    vec3_p GV_pVec3(PrOfs_e Param);
    ang3_p GV_pAng3(PrOfs_e Param);
    string_p GV_pqStr(PrOfs_e Param);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

    void initProgGlobals(progLump_t pl);

#ifdef __cplusplus
}
#endif