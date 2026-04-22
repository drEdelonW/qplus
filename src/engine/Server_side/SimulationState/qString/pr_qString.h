#pragma once
#include "types.h"
#include "pr_comp.h"

typedef int32_t string_t;
typedef int32_t qVmString_t;    // should be signed!!! it was [string_t] from "pr_comp.h"

#define E_STRING(e, o)  PR_GetQString(*(qVmString_t*)&((float_p)&(e)->v)[(o)])

#ifdef __cplusplus
extern "C" {
#endif

    void initProgSrting(dprograms_p progs);
    void PR_ClearAppStrings();

    qVmString_t PR_SetQString(cString str);
    cString PR_GetQString(qVmString_t offs);

#ifdef __cplusplus
}
#endif