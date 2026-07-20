#pragma once
#include "types.h"
#include "assert.h"
#include "VM_type_string.h"
#include "VM_param.h"   // MAX_PARMS

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

#define MAX_FUNCTIONS (0x4000) /* 8192 */
extern dFunction_t functions[MAX_FUNCTIONS];
extern int numfunctions;
