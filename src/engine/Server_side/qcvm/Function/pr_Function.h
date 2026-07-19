#pragma once

#include "types.h"
#include "assert.h"
#include "pr_qString.h"

#include "VM_type_func.h"

#define MAX_PARMS (8)

typedef struct {
    int32_t     first_statement; // negative numbers are builtins
    int32_t     parm_start;
    int32_t     locals;    // total ints of parms + locals

    int32_t     profile;  // runtime

    string_t    s_name;
    string_t    s_file;   // source file defined in

    int32_t     numparms;
    uint8_t     parm_size[MAX_PARMS];
} dFunction_t;      STATIC_ASSERT_SIZE(dFunction_t, 7*4 + 1*8); // 36
typedef dFunction_t* dFunction_p;

extern dFunction_p  pr_functions; // TODO: hide it

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
