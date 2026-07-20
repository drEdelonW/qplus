#pragma once

#define MAX_STATEMENTS (0x10000)/* 65536 */


#if 0 // original
typedef struct statement_s {
    unsigned short op;
    short a, b, c;
} dStatement_t;
#endif
#include "types.h"
#include "assert.h"

typedef uint16_t op_type;   // prog_operation_t // #include "pr_ops.h"
typedef int16_t arg_type;   // should be signed int

typedef struct {
    op_type     op;    // prog_operation_t
    arg_type    a;
    arg_type    b;
    arg_type    c;
} dStatement_t;  STATIC_ASSERT_SIZE(dStatement_t, 2*4);  // 8

typedef dStatement_t* dStatement_p;

extern int numstatements;
extern int statement_linenums[MAX_STATEMENTS];
extern dStatement_t statements[MAX_STATEMENTS];

