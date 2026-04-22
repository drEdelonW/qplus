#pragma once

#include "progdefs.h"
#include "progLump.h"

typedef globalvars_t* globalvars_p;
extern globalvars_p pr_global_struct;   // global variable of game settings
extern float_p      pr_globals;         // same as pr_global_struct

#define RETURN_EDICT(edict) (((int *)pr_globals)[OFS_RETURN] = ED_GetEDictOffs(edict))
#define G_FLOAT(o)          (pr_globals[(o)])
#define G_INT_P(o)          ((int32_p)&pr_globals[(o)])
#define G_INT(o)            (*(int32_p)&pr_globals[(o)])
#define G_VECTOR(o)         (&pr_globals[(o)])
#define G_STRING(o)         PR_GetQString(*(qVmString_t*)&pr_globals[(o)])

#ifdef __cplusplus
extern "C" {
#endif

    void initProgGlobals(TypeLess_ptr base, progLump_t pl);

#ifdef __cplusplus
}
#endif