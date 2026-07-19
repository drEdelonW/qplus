#pragma once
/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

// this file is shared by quake and qcc
#include "types.h"
#include "VM_param.h"
#include "VM_statment.h"
#include "VM_type_string.h"
typedef struct {
    uint16_t    type; // [etype_t] if DEF_SAVEGLOBAL bit is set the variable needs to be saved in savegames
    uint16_t    ofs;
    string_t    s_name;
} ddef_t;       STATIC_ASSERT_SIZE(ddef_t, 2*2 + 4); 
typedef ddef_t* ddef_p;
#define DEF_SAVEGLOBGAL (1 << 15)

#define MAX_PARMS 8

typedef struct {
    int  first_statement; // negative numbers are builtins
    int  parm_start;
    int  locals;    // total ints of parms + locals

    int  profile;  // runtime

    int  s_name;
    int  s_file;   // source file defined in

    int  numparms;
    uint8_t parm_size[MAX_PARMS];
} dfunction_t;
typedef dfunction_t* dfunction_p;


#define PROG_VERSION 6
typedef struct {
    int  version;
    int  crc;   // check of header file

// lumps
    int  ofs_statements;
    int  numstatements; // statement 0 is an error

    int  ofs_globaldefs;
    int  numglobaldefs;

    int  ofs_fielddefs;
    int  numfielddefs;

    int  ofs_functions;
    int  numfunctions; // function 0 is an empty

    int  ofs_strings;
    int  numstrings;  // first string is a null string

    int  ofs_globals;
    int  numglobals;

    int  entityfields;
} dprograms_t;

