#pragma once

#include "types.h"

// offsets are allways multiplied by 4 before using
typedef int gofs_t;    // offset in global data block

typedef struct def_s def_t;
typedef def_t* def_p;

typedef struct type_s type_t;
typedef type_t* type_p;

#include "VM_param.h"
#include "VM_enumTypes.h"
struct type_s {
    etype_t type;
    def_p   def;  // a def that points to this type
    type_p  next;
    // function types are more complex
    type_p  aux_type;  // return type or field type
    int     num_parms; // -1 = variable args
    type_p  parm_types[MAX_PARMS]; // only [num_parms] allocated
};


struct def_s {
    type_p  type;
    cStr_p  name;
    def_p   next;
    gofs_t  ofs;
    def_p   scope;  // function the var was defined in, or NULL
    int     initialized; // 1 when a declaration included "= immediate"
};


extern def_p def_for_type[ev_LAST];
extern cStringRO typeCName[ev_LAST];

extern type_t type_void;
extern type_t type_string;
extern type_t type_float;
extern type_t type_vector;
extern type_t type_entity;
extern type_t type_field;
extern type_t type_function;
extern type_t type_pointer;
extern type_t type_floatfield;

extern def_t def_void;
extern def_t def_string;
extern def_t def_float;
extern def_t def_vector;
extern def_t def_entity;
extern def_t def_field;
extern def_t def_function;
extern def_t def_pointer;


