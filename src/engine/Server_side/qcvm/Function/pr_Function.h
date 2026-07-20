#pragma once

#include "types.h"
#include "assert.h"

#include "Lump.h"
#include "VM_function.h"

#ifdef __cplusplus
extern "C" {
#endif

    dFunction_p ED_FindFunction(cString name);
    void initProgFunction(progLump_t pl);
    cString Get_xFnName();

    void FnPush(int32_p self);
    void FnPop(int32_p self);

#ifdef __cplusplus
}
#endif
