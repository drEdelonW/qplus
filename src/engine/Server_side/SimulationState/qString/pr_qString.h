#pragma once
#include "types.h"
#include "pr_comp.h"
#include "Lump.h"

typedef int32_t string_t;
typedef string_t* string_p;
typedef int32_t qVmString_t;    // should be signed!!! it was [string_t] from "pr_comp.h"

#define E_STRING(e, o)  PR_GetQString(*(qVmString_t*)&((float_p)&(e)->v)[(o)])

#ifdef __cplusplus
extern "C" {
#endif

    void initProgString(progLump_t pl);
    void PR_ClearAppStrings();

    qVmString_t PR_SetQString(cString str);
    cString PR_GetQString(qVmString_t offs);

#ifdef __cplusplus
}
#endif