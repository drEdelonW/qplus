#pragma once

#include "assert.h"
#include "types.h"          // for int32_t
#include "vector.h"         // for vec3_t
#include "VM_type_func.h"   // for func_t
#include "VM_type_string.h" // for string_t

typedef union {
    string_t    string;
    float       _float;
    vec3_t      vector;
    func_t      function;
    int32_t     _int;   // VM-slot as 32-bit
    int32_t     edict;  // 32-bit byte offset from GetEdictsPtr()
    // Any_p       ptr;    // not used
} eval_t;   STATIC_ASSERT_SIZE(eval_t, 3*4);    // 12
typedef eval_t* eval_p;
