#pragma once

#include "types.h"
#include "progdefs.h"


typedef union {
    string_t    string;
    float       _float;
    vec3_t      vector;
    func_t      function;
    int32_t     _int;    // VM-slot as 32-bit
    int32_t     edict;   // 32-bit byte offset from Edicts
} eval_t;
typedef eval_t* eval_p;
STATIC_ASSERT_SIZE(eval_t, 3*4);    // 12


typedef enum {
    ev_void     = 0u,
    ev_string,
    ev_float,
    ev_vector,
    ev_entity,
    ev_field,
    ev_function,
    ev_pointer,
    ev_LAST,

    DEF_SAVEGLOBAL = (1U << 15)
} etype_t;  // :uint16_t

extern int type_size[ev_LAST];

typedef int16_t arg_type;   // should be signed int

cString PR_ValueString(etype_t type, eval_p val);
cString PR_UglyValueString(etype_t type, eval_p val);