#pragma once


#include "types.h"
#include "VM_statment.h"    // dStatement_p
#include "Lump.h" // progLump_t
#ifdef __cplusplus
extern "C" {
#endif

    void initProgStatement(progLump_t pl);
    void PR_PrintStatement(dStatement_p state);
    dStatement_p PR_GetStack(int32_t stack);

#ifdef __cplusplus
}
#endif
