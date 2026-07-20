#include "VM_enumTypes.h"

#include "VM_type_string.h"
#include "VM_type_func.h"
#include "vector.h"
const int type_size[ev_LAST] = {
    [ev_void]    /**/ = 1,
    [ev_string]  /**/ = sizeof(string_t) / 4,
    [ev_float]   /**/ = 1,
    [ev_vector]  /**/ = sizeof(vec3_t) / 4,
    [ev_entity]  /**/ = 1,
    [ev_field]   /**/ = 1,
    [ev_function]/**/ = sizeof(func_t) / 4,
    [ev_pointer] /**/ = sizeof(TypeLess_ptr) / 4
};