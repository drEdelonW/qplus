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
#include "progdefs.h"


typedef union {
    string_t    string;
    float       _float;
    vec3_t      vector;
    func_t      function;
    int32_t     _int;    // VM-slot as 32-bit
    int32_t     edict;   // 32-bit byte offset from sv.edicts
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

typedef struct {
    uint16_t    type;   // [etype_t] if DEF_SAVEGLOBAL bit is set
                        // the variable needs to be saved in savegames
    uint16_t    ofs;
    string_t    s_name;
} dDef_t;
typedef dDef_t* dDef_p;
STATIC_ASSERT_SIZE(dDef_t, 2*2 + 4);    // 8


#define MAX_PARMS (8)

typedef struct {
    int32_t     first_statement; // negative numbers are builtins
    int32_t     parm_start;
    int32_t     locals;    // total ints of parms + locals

    int32_t     profile;  // runtime

    string_t    s_name;
    string_t    s_file;   // source file defined in

    int32_t     numparms;
    uint8_t     parm_size[MAX_PARMS];
} dFunction_t;
typedef dFunction_t* dFunction_p;
STATIC_ASSERT_SIZE(dFunction_t, 7*4 + 1*8); // 36



#define PROG_VERSION 6
typedef struct {
    uint32_t ofs;   /* byte offset from start of progs blob */
    uint32_t num;   /* element count (not bytes) */
} progLump_t;

typedef struct {
    int32_t  version;
    int32_t  crc;   // check of header file

    progLump_t statements;   // statement 0 is an error
    progLump_t globaldefs;
    progLump_t fielddefs;
    progLump_t functions;    // function 0 is an empty
    progLump_t strings;      // first string is a null string
    progLump_t globals;

    uint32_t  entityfields;
} dprograms_t;
typedef dprograms_t* dprograms_p;
STATIC_ASSERT_SIZE(dprograms_t, 2*4 + 6*8 + 4); // 60
