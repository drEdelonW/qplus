#pragma once
#include "pr_comp.h"
#include "Lump.h"   // progLump_t
#include "VM_type_string.h" // qVmString_t
#define E_STRING(e, o)  PR_GetQString(*(qVmString_t*)&((float_p)&(e)->v)[(o)])

#ifdef __cplusplus
extern "C" {
#endif

    void initProgString(progLump_t pl);
    void PR_ClearAppStrings();

    qVmString_t PR_SetQString(cStr_p str);
    cStr_p PR_GetQString(qVmString_t offs);

#ifdef __cplusplus
}
#endif