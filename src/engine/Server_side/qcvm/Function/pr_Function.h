#pragma once

#include "types.h"
#include "assert.h"
#include "pr_qString.h"

typedef int32_t func_t;

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
} dFunction_t;
typedef dFunction_t* dFunction_p;
STATIC_ASSERT_SIZE(dFunction_t, 7*4 + 1*8); // 36

extern dFunction_p  pr_functions;
extern dFunction_p  pr_xFunction;

#ifdef __cplusplus
extern "C" {
#endif

    dFunction_p ED_FindFunction(cString name);

#ifdef __cplusplus
}
#endif
