#include "VM_types.h"

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

def_p def_for_type[ev_LAST] = {
    [ev_void]    /**/ = &def_void,
    [ev_string]  /**/ = &def_string,
    [ev_float]   /**/ = &def_float,
    [ev_vector]  /**/ = &def_vector,
    [ev_entity]  /**/ = &def_entity,
    [ev_field]   /**/ = &def_field,
    [ev_function]/**/ = &def_function,
    [ev_pointer] /**/ = &def_pointer
};

cStringRO typeCName[ev_LAST] = {
#if 0
    [ev_void]    /**/ = "void     ",
    [ev_string]  /**/ = "string_t ",
    [ev_float]   /**/ = "float    ",
    [ev_vector]  /**/ = "vec3_t   ",
    [ev_entity]  /**/ = "EdIdx    ",
    [ev_field]   /**/ = "",
    [ev_function]/**/ = "func_t   ",
    [ev_pointer] /**/ = "void*    "
#else
    [ev_void]    /**/ = "void     ",
    [ev_string]  /**/ = "string_t ",
    [ev_float]   /**/ = "float    ",
    [ev_vector]  /**/ = "vec3_t   ",
    [ev_entity]  /**/ = "int      ",
    [ev_field]   /**/ = "skipped  ",
    [ev_function]/**/ = "func_t   ",
    [ev_pointer] /**/ = "int      "
#endif
};