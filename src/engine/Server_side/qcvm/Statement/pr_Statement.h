#pragma once

#include "vmValue.h"

#include "types.h"
#include "assert.h"

typedef uint16_t op_type;
typedef int16_t arg_type;   // should be signed int

typedef struct {
    op_type     op;    // prog_operation_e
    arg_type    a;
    arg_type    b;
    arg_type    c;
} dStatement_t;
typedef dStatement_t* dStatement_p;
STATIC_ASSERT_SIZE(dStatement_t, 2*4);  // 8



#ifdef __cplusplus
extern "C" {
#endif

    void initProgStatement(TypeLess_ptr base, progLump_t pl);
    void PR_PrintStatement(dStatement_p state);
    dStatement_p PR_GetStack(int32_t stack);

#ifdef __cplusplus
}
#endif
