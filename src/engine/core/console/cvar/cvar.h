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
// cvar.h

/*

cvar_t variables are used to hold scalar or string variables that can be changed or displayed at the console or prog code as well as accessed directly
in C code.

it is sufficient to initialize a cvar_t with just the first two fields, or
you can add a ,true flag for variables that you want saved to the configuration
file when the game is quit:

cvar_t    r_draworder = {"r_draworder", "1"};
cvar_t    scr_screensize = {"screensize", "1",true};

Cvars must be registered before use, or they will have a 0 value instead of the float interpretation of the string.  Generally, all cvar_t declarations should be registered in the apropriate init function before any console commands are executed:
Cvar_RegisterVariable(&host_framerate);


C code usually just references a cvar in place:
if ( r_draworder.value )

It could optionally ask for the value to be looked up for a string name:
if (Cvar_VariableValue ("r_draworder"))

Interpreted prog code can access cvars with the cvar(name) or
cvar_set (name, value) internal functions:
teamplay = cvar("teamplay");
cvar_set ("registered", "1");

The user can access cvars from the console in two ways:
r_draworder            prints the current value
r_draworder 0        sets the current value to 0
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.
*/
#include "qboolean.h"
#include <stdio.h>
#include <stdint.h>
#include "zone.h"

/* header-friendly extern */
#define CVAR_EXTERN(sym) extern cvar_t sym

/* 2-field definition: symbol + default string.
   name is taken from symbol via stringification. value is 0 until Cvar_RegisterVariable() */
#define CVAR(sym, defstr) \
    cvar_t sym = { #sym, (cstring)(defstr), (uint8_t)cvf_none, 0.0f, NULL }
//  reg_srch: ^\s*cvar_t\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*"\1"\s*,\s*"([^"]*)"\s*\}\s*;
//  reg_rpc:  CVAR($1, "$2");

/* same, but with flags */
#define CVAR_F(sym, defstr, fl) \
    cvar_t sym = { #sym, (cstring)(defstr), (uint8_t)(fl), 0.0f, NULL }

/* explicit cvar name (if it differs from the C symbol) */
#define CVAR_NAMED(sym, namestr, defstr) \
   cvar_t sym = { (cstring)(namestr), (cstring)(defstr), (uint8_t)cvf_none, 0.0f, NULL }
//  reg_srch: ^\s*cvar_t\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*\{\s*"([^"]+)"\s*,\s*"([^"]*)"\s*\}\s*;
//  reg_rpc:  CVAR_NAMED($1, "$2", "$3", cvf_none);

#define CVAR_NAMED_AR(sym, namestr, defstr) \
   cvar_t sym = { (cstring)(namestr), (cstring)(defstr), (uint8_t)cvf_archive, 0.0f, NULL }

/* config-style helper: name == "_" "symbol", archived */
#define CVAR_CFG(sym, defstr) \
    cvar_t sym = { "_" #sym, (cstring)(defstr), (uint8_t)cvf_archive, 0.0f, NULL }


typedef enum{
    cvf_none           = 0,
    cvf_archive        = 1 << 0,   // be saved to vars.rc
    cvf_server         = 1 << 1,   // notifies players when changed
    cvf_archive_server = cvf_archive | cvf_server,   // both: be saved to vars.rc and notifies players when changed

    cvf_all            = 0xFF,
} cvar_flags_t;

/* convenience shorthands */
#define CVAR_SV(sym, defstr)    CVAR_F(sym, defstr, cvf_server)
//  reg_srch: ^\s*cvar_t\s+([A-Za-z_]\w*)\s*=\s*\{\s*"\1"\s*,\s*"([^"]*)"\s*,\s*false\s*,\s*true\s*\}\s*;(\s*//.*)?$
//  reg_rpc:  CVAR_SV($1, "$2");$3

#define CVAR_ARC(sym, defstr)   CVAR_F(sym, defstr, cvf_archive)
//  reg_srch: ^\s*cvar_t\s+([A-Za-z_]\w*)\s*=\s*\{\s*"\1"\s*,\s*"([^"]*)"\s*,\s*true\s*\}\s*;(\s*//.*)?$
//  reg_rpc:  CVAR_ARC($1, "$2");

#define CVAR_AS(sym, defstr)    CVAR_F(sym, defstr, cvf_archive_server)
//  reg_srch: ^\s*cvar_t\s+([A-Za-z_]\w*)\s*=\s*\{\s*"\1"\s*,\s*"([^"]*)"\s*,\s*true\s*,\s*true\s*\}\s*;(\s*//.*)?$
//  reg_rpc:  CVAR_AS($1, "$2");$3

typedef struct cvar_s{
    char    *name;
    char    *string;
    // qboolean archive;        // set to true to cause it to be saved to vars.rc
    // qboolean server;        // notifies players when changed
    uint8_t  flags;
    float    value;             // cached numeric value
    struct cvar_s *next;
} cvar_t;

// registers a cvar that allready has the name, string, and optionally the
// archive elements set.
void     Cvar_RegisterVariable(cvar_t *variable);

// equivelant to "<name> <variable>" typed at the console
void     Cvar_Set (char *var_name, char *value);

// expands value to a string and calls Cvar_Set
void    Cvar_SetValue (char *var_name, float value);

// returns 0 if not defined or non numeric
float    Cvar_VariableValue (char *var_name);

// returns an empty string if not defined
char    *Cvar_VariableString (char *var_name);

// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits
char     *Cvar_CompleteVariable (char *partial);

// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)
qboolean Cvar_Command();

// Writes lines containing "set variable value" for all variables
// with the archive flag set to true.
void     Cvar_WriteVariables (FILE *f);

cvar_t *Cvar_FindVar (char *var_name);

extern cvar_t    *cvar_vars;
