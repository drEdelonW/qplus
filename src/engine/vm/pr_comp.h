#pragma once
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// this file is shared by quake and qcc
#include "types.h"

typedef int32_t func_t;
typedef int32_t string_t;

typedef enum {
    ev_void,
    ev_string,
    ev_float,
    ev_vector,
    ev_entity,
    ev_field,
    ev_function,
    ev_pointer
} etype_t;


#define OFS_NULL  0
#define OFS_RETURN  1
#define OFS_PARM0  4  // leave 3 ofs for each parm to hold vectors
#define OFS_PARM1  7
#define OFS_PARM2  10
#define OFS_PARM3  13
#define OFS_PARM4  16
#define OFS_PARM5  19
#define OFS_PARM6  22
#define OFS_PARM7  25
#define RESERVED_OFS 28


typedef enum {
    OP_DONE,
    OP_MUL_F,
    OP_MUL_V,
    OP_MUL_FV,
    OP_MUL_VF,
    OP_DIV_F,
    OP_ADD_F,
    OP_ADD_V,
    OP_SUB_F,
    OP_SUB_V,

    OP_EQ_F,
    OP_EQ_V,
    OP_EQ_S,
    OP_EQ_E,
    OP_EQ_FNC,

    OP_NE_F,
    OP_NE_V,
    OP_NE_S,
    OP_NE_E,
    OP_NE_FNC,

    OP_LE,
    OP_GE,
    OP_LT,
    OP_GT,

    OP_LOAD_F,
    OP_LOAD_V,
    OP_LOAD_S,
    OP_LOAD_ENT,
    OP_LOAD_FLD,
    OP_LOAD_FNC,

    OP_ADDRESS,

    OP_STORE_F,
    OP_STORE_V,
    OP_STORE_S,
    OP_STORE_ENT,
    OP_STORE_FLD,
    OP_STORE_FNC,

    OP_STOREP_F,
    OP_STOREP_V,
    OP_STOREP_S,
    OP_STOREP_ENT,
    OP_STOREP_FLD,
    OP_STOREP_FNC,

    OP_RETURN,
    OP_NOT_F,
    OP_NOT_V,
    OP_NOT_S,
    OP_NOT_ENT,
    OP_NOT_FNC,
    OP_IF,
    OP_IFNOT,
    OP_CALL0,
    OP_CALL1,
    OP_CALL2,
    OP_CALL3,
    OP_CALL4,
    OP_CALL5,
    OP_CALL6,
    OP_CALL7,
    OP_CALL8,
    OP_STATE,
    OP_GOTO,
    OP_AND,
    OP_OR,

    OP_BITAND,
    OP_BITOR,
    OP_LAST     // Should be last
} prog_operation_e;
extern cstring pr_opnames[OP_LAST];

typedef uint16_t op_type;
typedef int16_t arg_type;
typedef struct statement_s {
    op_type     op;    // prog_operation_e
    arg_type    a;
    arg_type    b;
    arg_type    c;
} dstatement_t;
typedef dstatement_t* dstatement_p;

typedef struct {
    uint16_t type;  // if DEF_SAVEGLOBGAL bit is set
    // the variable needs to be saved in savegames
    uint16_t ofs;
    int32_t   s_name;
} ddef_t;
typedef ddef_t* ddef_p;

#define DEF_SAVEGLOBAL (1 << 15)
#define MAX_PARMS (8)

typedef struct {
    int32_t  first_statement; // negative numbers are builtins
    int32_t  parm_start;
    int32_t  locals;    // total ints of parms + locals

    int32_t  profile;  // runtime

    int32_t  s_name;
    int32_t  s_file;   // source file defined in

    int32_t  numparms;
    uint8_t parm_size[MAX_PARMS];
} dfunction_t;
typedef dfunction_t* dfunction_p;


#define PROG_VERSION 6
typedef struct {
    int32_t ofs;   /* byte offset from start of progs blob */
    int32_t num;   /* element count (not bytes) */
} prog_lump32_t;

typedef struct {
    int32_t  version;
    int32_t  crc;   // check of header file

    prog_lump32_t statements;   // statement 0 is an error
    prog_lump32_t globaldefs;
    prog_lump32_t fielddefs;
    prog_lump32_t functions;    // function 0 is an empty
    prog_lump32_t strings;      // first string is a null string
    prog_lump32_t globals;

    int32_t  entityfields;
} dprograms_t;
typedef dprograms_t* dprograms_p;

