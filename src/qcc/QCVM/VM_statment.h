#pragma once

#include "types.h"
#include "assert.h"


#if 0 // original
typedef struct statement_s {
    unsigned short op;
    short a, b, c;
} dStatement_t;
#endif
#include "assert.h"
typedef uint16_t op_type;   // prog_operation_e // #include "pr_ops.h"
typedef int16_t arg_type;   // should be signed int

typedef struct {
    op_type     op;    // prog_operation_e
    arg_type    a;
    arg_type    b;
    arg_type    c;
} dStatement_t;  STATIC_ASSERT_SIZE(dStatement_t, 2*4);  // 8

typedef dStatement_t* dStatement_p;
