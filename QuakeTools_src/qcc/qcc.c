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

#include "qcc.h"
#include <unistd.h>
#include "CLAMP.h"

char  destfile[1024];

float  pr_globals[MAX_REGS];
int   numpr_globals;

char  strings[MAX_STRINGS];
int   strofs;

dstatement_t statements[MAX_STATEMENTS];
int   numstatements;
int   statement_linenums[MAX_STATEMENTS];

dfunction_t functions[MAX_FUNCTIONS];
int   numfunctions;

ddef_t  globals[MAX_GLOBALS];
int   numglobaldefs;

ddef_t  fields[MAX_FIELDS];
int   numfielddefs;

char  precache_sounds[MAX_SOUNDS][MAX_DATA_PATH];
int   precache_sounds_block[MAX_SOUNDS];
int   numsounds;

char  precache_models[MAX_MODELS][MAX_DATA_PATH];
int   precache_models_block[MAX_SOUNDS];
int   nummodels;

char  precache_files[MAX_FILES][MAX_DATA_PATH];
int   precache_files_block[MAX_SOUNDS];
int   numfiles;


/*
=================
BspModels

Runs qbsp and light on all of the models with a .bsp extension
=================
*/
void BspModels() {
    int p = CheckParm("-bspmodels");
    if (!p)
        return;
    if (p == (myargc - 1))
        Error("-bspmodels must preceed a game directory");
    cStr_p gamedir = myargv[p + 1];

    for (int i = 0; i < nummodels; i++) {
        cStr_p m = precache_models[i];
        if (strcmp(m + strlen(m) - 4, ".bsp"))
            continue;

        char name[256];
        strcpy(name, m);
        name[strlen(m) - 4] = 0;
        char cmd[1024];
        snprintf(cmd,
            sizeof(cmd),
            "qbsp %s/%s ; light -extra %s/%s",
            gamedir, name, gamedir, name
        );
        system(cmd);
    }
}

// CopyString returns an offset from the string heap
int CopyString(cStr_p str) {
    int old = strofs;
    strcpy(strings + strofs, str);
    strofs += strlen(str) + 1;
    return old;
}

void PrintStrings() {
    int l = 0;
    for (int i = 0; i < strofs; i += l) {
        l = strlen(strings + i) + 1;
        printf("%5i : ", i);
        for (int j = 0; j < l; j++) {
            if (strings[i + j] == '\n') {
                putchar('\\');  putchar('n');
            }
            else
                putchar(strings[i + j]);
        }
        printf("\n");
    }
}


void PrintFunctions() {
    for (int i = 0; i < numfunctions; i++) {
        dfunction_p d = &functions[i];
        printf(
            "%s : %s : %i %i (",
            strings + d->s_file,
            strings + d->s_name,
            d->first_statement,
            d->parm_start
        );
        for (int j = 0; j < d->numparms; j++)
            printf("%i ", d->parm_size[j]);
        printf(")\n");
    }
}

void PrintFields() {
    for (int i = 0; i < numfielddefs; i++) {
        ddef_p d = &fields[i];
        printf(
            "%5i : (%i) %s\n",
            d->ofs,
            d->type,
            strings + d->s_name
        );
    }
}

void PrintGlobals() {
    for (int i = 0; i < numglobaldefs; i++) {
        ddef_p d = &globals[i];
        printf(
            "%5i : (%i) %s\n",
            d->ofs,
            d->type,
            strings + d->s_name
        );
    }
}


void InitData() {
    numstatements = 1;
    strofs = 1;
    numfunctions = 1;
    numglobaldefs = 1;
    numfielddefs = 1;

    def_ret.ofs = OFS_RETURN;
    for (int i = 0; i < MAX_PARMS; i++)
        def_parms[i].ofs = OFS_PARM0 + 3 * i;
}


void WriteData(int crc) {
    for (def_p def = pr.def_head.next; def; def = def->next) {
        ddef_p dd;
        /**/ if (def->type->type == ev_function) {
            //   df = &functions[numfunctions];
            //   numfunctions++;
        }
        else if (def->type->type == ev_field) {
            dd = &fields[numfielddefs];
            numfielddefs++;
            dd->type = def->type->aux_type->type;
            dd->s_name = CopyString(def->name);
            dd->ofs = G_INT(def->ofs);
        }
        dd = &globals[numglobaldefs];
        numglobaldefs++;
        dd->type = def->type->type;
        if (!(def->initialized) &&
            (def->type->type != ev_function) &&
            (def->type->type != ev_field) &&
            (def->scope == NULL)
            )   dd->type |= DEF_SAVEGLOBAL;
        dd->s_name = CopyString(def->name);
        dd->ofs = def->ofs;
    }

    //PrintStrings ();
    //PrintFunctions ();
    //PrintFields ();
    //PrintGlobals ();
    strofs = (strofs + 3) & ~3;

    printf("%6i strofs\n", strofs);
    printf("%6i numstatements\n", numstatements);
    printf("%6i numfunctions\n", numfunctions);
    printf("%6i numglobaldefs\n", numglobaldefs);
    printf("%6i numfielddefs\n", numfielddefs);
    printf("%6i numpr_globals\n", numpr_globals);

    int h = SafeOpenWrite(destfile);
    dprograms_t progs;
    SafeWrite(h, &progs, sizeof(progs));

    progs.ofs_strings = lseek(h, 0, SEEK_CUR);
    progs.numstrings = strofs;
    SafeWrite(h, strings, strofs);

    progs.ofs_statements = lseek(h, 0, SEEK_CUR);
    progs.numstatements = numstatements;
    for (int i = 0; i < numstatements; i++) {
        statements[i].op = LittleShort(statements[i].op);
        statements[i].a = LittleShort(statements[i].a);
        statements[i].b = LittleShort(statements[i].b);
        statements[i].c = LittleShort(statements[i].c);
    }
    SafeWrite(h, statements, numstatements * sizeof(dstatement_t));

    progs.ofs_functions = lseek(h, 0, SEEK_CUR);
    progs.numfunctions = numfunctions;
    for (int i = 0; i < numfunctions; i++) {
        functions[i].first_statement = LittleLong(functions[i].first_statement);
        functions[i].parm_start = LittleLong(functions[i].parm_start);
        functions[i].s_name = LittleLong(functions[i].s_name);
        functions[i].s_file = LittleLong(functions[i].s_file);
        functions[i].numparms = LittleLong(functions[i].numparms);
        functions[i].locals = LittleLong(functions[i].locals);
    }
    SafeWrite(h, functions, numfunctions * sizeof(dfunction_t));

    progs.ofs_globaldefs = lseek(h, 0, SEEK_CUR);
    progs.numglobaldefs = numglobaldefs;
    for (int i = 0; i < numglobaldefs; i++) {
        globals[i].type = LittleShort(globals[i].type);
        globals[i].ofs = LittleShort(globals[i].ofs);
        globals[i].s_name = LittleLong(globals[i].s_name);
    }
    SafeWrite(h, globals, numglobaldefs * sizeof(ddef_t));

    progs.ofs_fielddefs = lseek(h, 0, SEEK_CUR);
    progs.numfielddefs = numfielddefs;
    for (int i = 0; i < numfielddefs; i++) {
        fields[i].type = LittleShort(fields[i].type);
        fields[i].ofs = LittleShort(fields[i].ofs);
        fields[i].s_name = LittleLong(fields[i].s_name);
    }
    SafeWrite(h, fields, numfielddefs * sizeof(ddef_t));

    progs.ofs_globals = lseek(h, 0, SEEK_CUR);
    progs.numglobals = numpr_globals;
    for (int i = 0; i < numpr_globals; i++)
        ((int*)pr_globals)[i] = LittleLong(((int*)pr_globals)[i]);
    SafeWrite(h, pr_globals, numpr_globals * 4);

    printf(
        "%6i TOTAL SIZE\n",
        (int)lseek(h, 0, SEEK_CUR)
    );

    progs.entityfields = pr.size_fields;

    progs.version = PROG_VERSION;
    progs.crc = crc;

    // byte swap the header and write it out
    for (int i = 0; i < sizeof(progs) / 4; i++)
        ((int*)&progs)[i] = LittleLong(((int*)&progs)[i]);
    lseek(h, 0, SEEK_SET);
    SafeWrite(h, &progs, sizeof(progs));
    close(h);
}



/*
===============
PR_String

Returns a string suitable for printing (no newlines, max 60 chars length)
===============
*/
cStr_p PR_String(cStr_p string) {
    static char buf[80];
    cStr_p s = buf;
    *s++ = '"';
    while ((string) &&
        (*string)
        ) {
        if (s == (buf + sizeof(buf) - 2))
            break;
        if (*string == '\n') {
            *s++ = '\\';
            *s++ = 'n';
        }
        else if (*string == '"') {
            *s++ = '\\';
            *s++ = '"';
        }
        else
            *s++ = *string;
        string++;
        if ((s - buf) > 60) {
            *s++ = '.';
            *s++ = '.';
            *s++ = '.';
            break;
        }
    }
    *s++ = '"';
    *s++ = 0;
    return buf;
}



def_p PR_DefForFieldOfs(gofs_t ofs) {
    for (def_p d = pr.def_head.next; d; d = d->next) {
        if (d->type->type != ev_field)              continue;
        if (*((int*)&pr_globals[d->ofs]) == ofs)    return d;
    }
    Error("PR_DefForFieldOfs: couldn't find %i", ofs);
    return NULL;
}

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
cStr_p PR_ValueString(etype_t type, Any_p val) {
    static char line[256];
    switch (type) {
    case ev_string:     snprintf(line, sizeof(line), "%s", PR_String(strings + *(int*)val));    break;
    case ev_entity:     snprintf(line, sizeof(line), "entity %i", *(int*)val);                  break;
    case ev_function: {
        dfunction_p f = functions + *(int*)val;
        if (!f)         snprintf(line, sizeof(line), "undefined function");
        else            snprintf(line, sizeof(line), "%s()", strings + f->s_name);
    } break;
    case ev_field:      snprintf(line, sizeof(line), ".%s", (PR_DefForFieldOfs(*(int*)val))->name);   break;
    case ev_void:       snprintf(line, sizeof(line), "void");        break;
    case ev_float:      snprintf(line, sizeof(line), "%5.1f", *(float*)val);        break;
    case ev_vector:     snprintf(line, sizeof(line), "'%5.1f %5.1f %5.1f'", ((float*)val)[0], ((float*)val)[1], ((float*)val)[2]);        break;
    case ev_pointer:    snprintf(line, sizeof(line), "pointer");        break;
    default:            snprintf(line, sizeof(line), "bad type %i", type);        break;
    }

    return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
cStr_p PR_GlobalStringNoContents(gofs_t ofs) {
    static char line[128];
    def_p def = pr_global_defs[ofs];
    if (!def)   snprintf(line, sizeof(line), "%i(???)", ofs);    //  Error ("PR_GlobalString: no def for %i", ofs);
    else        snprintf(line, sizeof(line), "%i(%s)", ofs, def->name);

    int i = strlen(line);
    for (; i < 16; i++)
        strcat(line, " ");
    strcat(line, " ");

    return line;
}

cStr_p PR_GlobalString(gofs_t ofs) {
    def_p def = pr_global_defs[ofs];
    if (!def)
        return PR_GlobalStringNoContents(ofs);

    static char line[128];
    if ((def->initialized) &&
        (def->type->type != ev_function)
        )       snprintf(line, sizeof(line), "%i(%s)", ofs, PR_ValueString(def->type->type, &pr_globals[ofs]));
    else        snprintf(line, sizeof(line), "%i(%s)", ofs, def->name);

    for (int i = strlen(line); i < 16; i++)
        strcat(line, " ");
    strcat(line, " ");

    return line;
}

/*
============
PR_PrintOfs
============
*/
void PR_PrintOfs(gofs_t ofs) {
    printf("%s\n", PR_GlobalString(ofs));
}

/*
=================
PR_PrintStatement
=================
*/
void PR_PrintStatement(dstatement_p s) {
    printf("%4i : %4i : %s ",
        (int)(s - statements),
        statement_linenums[s - statements],
        pr_opcodes[s->op].opname
    );
    int i = strlen(pr_opcodes[s->op].opname);
    for (; i < 10; i++)
        printf(" ");

    /**/ if ((s->op == OP_IF) || (s->op == OP_IFNOT))   printf("%sbranch %i", PR_GlobalString(s->a), s->b);
    else if (s->op == OP_GOTO)                          printf("branch %i", s->a);
    else if ((unsigned)(s->op - OP_STORE_F) < 6) {
        printf("%s", PR_GlobalString(s->a));
        printf("%s", PR_GlobalStringNoContents(s->b));
    }
    else {
        if (s->a)   printf("%s", PR_GlobalString(s->a));
        if (s->b)   printf("%s", PR_GlobalString(s->b));
        if (s->c)   printf("%s", PR_GlobalStringNoContents(s->c));
    }
    printf("\n");
}


/*
============
PR_PrintDefs
============
*/
void PR_PrintDefs() {
    for (def_p d = pr.def_head.next; d; d = d->next)
        PR_PrintOfs(d->ofs);
}


/*
==============
PR_BeginCompilation

called before compiling a batch of files, clears the pr struct
==============
*/
void PR_BeginCompilation(Any_p memory, int memsize) {
    pr.memory = memory;
    pr.max_memory = memsize;

    numpr_globals = RESERVED_OFS;
    pr.def_tail = &pr.def_head;

    for (int i = 0; i < RESERVED_OFS; i++)
        pr_global_defs[i] = &def_void;

    // link the function type in so state forward declarations match proper type
    pr.types = &type_function;
    type_function.next = NULL;
    pr_error_count = 0;
}

/*
==============
PR_FinishCompilation

called after all files are compiled to check for errors
Returns false if errors were detected.
==============
*/
bool PR_FinishCompilation() {
    bool errors = false;
    // check to make sure all functions prototyped have code
    for (def_p d = pr.def_head.next; d; d = d->next)
        if (d->type->type == ev_function && !d->scope) {// function parms are ok
            //   f = G_FUNCTION(d->ofs);
            //   if (!f || (!f->code && !f->builtin) )
            if (!d->initialized) {
                printf("function %s was not defined\n", d->name);
                errors = true;
            }
        }

    return !errors;
}

//=============================================================================

// FIXME: byte swap?

// this is a 16 bit, non-reflected CRC using the polynomial 0x1021
// and the initial and final xor values shown below...  in other words, the
// CCITT standard CRC used by XMODEM

typedef uint16_t CRC_t;
typedef CRC_t* CRC_p;
#define CRC_INIT_VALUE (CRC_t)0xFFFF
#define CRC_XOR_VALUE  (CRC_t)0x0000

static const CRC_t crctable[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

void CRC_Init(CRC_p crcvalue) { *crcvalue = CRC_INIT_VALUE; }

void CRC_ProcessByte(CRC_p crcvalue, uint8_t data) { *crcvalue = (*crcvalue << 8) ^ crctable[(*crcvalue >> 8) ^ data]; }

uint8_t CRC_Value(uint8_t crcvalue) { return crcvalue ^ CRC_XOR_VALUE; }
//=============================================================================

/*
============
PR_WriteProgdefs

Writes the global and entity structures out
Returns a crc of the header, to be stored in the progs file for comparison
at load time.
============
*/
int PR_WriteProgdefs(cStr_p filename) {
    printf("writing %s\n", filename);
    FILE* f = fopen(filename, "w"); {
        // print global vars until the first field is defined
        fprintf(f,
            "\n/* file generated by qcc, do not modify */\n\n"
            "typedef struct\n{\tint\tpad[%i];\n",
            RESERVED_OFS
        ); {
            for (def_p d = pr.def_head.next; d; d = d->next) {
                if (!strcmp(d->name, "end_sys_globals"))    break;

                switch (d->type->type) {
                case ev_float:      fprintf(f, "\tfloat\t%s;\n", d->name);      break;
                case ev_vector: {
                    fprintf(f, "\tvec3_t\t%s;\n", d->name); d = d->next->next->next; // skip the elements
                } break;
                case ev_string:     fprintf(f, "\tstring_t\t%s;\n", d->name);   break;
                case ev_function:   fprintf(f, "\tfunc_t\t%s;\n", d->name);     break;
                case ev_entity:     fprintf(f, "\tint\t%s;\n", d->name);        break;
                default:            fprintf(f, "\tint\t%s;\n", d->name);        break;
                }
            }
        } fprintf(f, "} globalvars_t;\n\n");

        // print all fields
        fprintf(f, "typedef struct\n{\n"); {
            for (def_p d = pr.def_head.next; d; d = d->next) {
                if (!strcmp(d->name, "end_sys_fields"))
                    break;

                if (d->type->type != ev_field)  continue;

                switch (d->type->aux_type->type) {
                case ev_float:      fprintf(f, "\tfloat\t%s;\n", d->name);      break;
                case ev_vector:
                    fprintf(f, "\tvec3_t\t%s;\n", d->name); d = d->next->next->next; // skip the elements
                    break;
                case ev_string:     fprintf(f, "\tstring_t\t%s;\n", d->name);   break;
                case ev_function:   fprintf(f, "\tfunc_t\t%s;\n", d->name);     break;
                case ev_entity:     fprintf(f, "\tint\t%s;\n", d->name);        break;
                default:            fprintf(f, "\tint\t%s;\n", d->name);        break;
                }
            }
        } fprintf(f, "} entvars_t;\n\n");
    } fclose(f);

    // do a crc of the file
    CRC_t crc;
    CRC_Init(&crc);
    f = fopen(filename, "r+"); {
        int c;
        while ((c = fgetc(f)) != EOF)
            CRC_ProcessByte(&crc, c);
        fprintf(f, "#define PROGHEADER_CRC %i\n", crc);
    } fclose(f);
    return crc;
}

void PrintFunction(cStr_p name) {
    int i = 0;
    for (; i < numfunctions; i++)
        if (!strcmp(name, strings + functions[i].s_name))
            break;
    if (i == numfunctions)
        Error("No function names \"%s\"", name);
    dfunction_p df = functions + i;

    printf("Statements for %s:\n", name);
    dstatement_p ds = statements + df->first_statement;
    while (1) {
        PR_PrintStatement(ds);
        if (!ds->op)
            break;
        ds++;
    }
}

/*
==============================================================================

DIRECTORY COPYING / PACKFILE CREATION

==============================================================================
*/

typedef struct {
    char name[56];
    int  filepos, filelen;
} packfile_t;
typedef packfile_t* packfile_p;

typedef struct {
    char id[4];
    int  dirofs;
    int  dirlen;
} packheader_t;

packfile_t pfiles[4096];
packfile_p pf;
int   packhandle;
int   packbytes;

void Sys_mkdir(cStr_p path) {
    if (mkdir(path, 0777) != -1)    return;
    if (errno != EEXIST)            Error("mkdir %s: %s", path, strerror(errno));
}

/*
============
CreatePath
============
*/
void CreatePath(cStr_p path) {
    for (cStr_p ofs = path + 1; *ofs; ofs++)
        if (*ofs == '/') { // create the directory
            *ofs = 0x00;
            Sys_mkdir(path);
            *ofs = '/';
        }
}


/*
===========
PackFile

Copy a file into the pak file
===========
*/
void PackFile(cStr_p src, cStr_p name) {
    if (((uint8_p)pf - (uint8_p)pfiles) > sizeof(pfiles))
        Error("Too many files in pak file");

    int in = SafeOpenRead(src);
    int remaining = filelength(in);

    pf->filepos = LittleLong(lseek(packhandle, 0, SEEK_CUR));
    pf->filelen = LittleLong(remaining);
    strcpy(pf->name, name);
    printf("%64s : %7i\n", pf->name, remaining);

    packbytes += remaining;

    while (remaining) {
        char buf[4096];
        int count = SmallerOf(remaining, sizeof(buf));
        SafeRead(in, buf, count);
        SafeWrite(packhandle, buf, count);
        remaining -= count;
    }

    close(in);
    pf++;
}


/*
===========
CopyFile

Copies a file, creating any directories needed
===========
*/
void CopyFile(cStr_p src, cStr_p dest) {
    printf("%s to %s\n", src, dest);

    int in = SafeOpenRead(src);
    int remaining = filelength(in);

    CreatePath(dest);
    int out = SafeOpenWrite(dest);

    while (remaining) {
        char buf[4096];
        int count = SmallerOf(remaining, sizeof(buf));
        SafeRead(in, buf, count);
        SafeWrite(out, buf, count);
        remaining -= count;
    }

    close(in);
    close(out);
}


/*
===========
CopyFiles
===========
*/
void CopyFiles() {
    printf("%3i unique precache_sounds\n", numsounds);
    printf("%3i unique precache_models\n", nummodels);

    int copytype = 0;

    char srcdir[1024];
    char destdir[1024];
    {
        int p = CheckParm("-copy");
        if ((p) &&
            (p < (myargc - 2))
            ) { // create a new directory tree
            copytype = 1;

            strcpy(srcdir, myargv[p + 1]);
            strcpy(destdir, myargv[p + 2]);
            if (srcdir[strlen(srcdir) - 1] != '/')
                strcat(srcdir, "/");
            if (destdir[strlen(destdir) - 1] != '/')
                strcat(destdir, "/");
        }
    }
    int blocknum = 1;
    packheader_t header;
    {
        int p = CheckParm("-pak2");
        if ((p) &&
            (p < (myargc - 2))
            )   blocknum = 2;
        else    p = CheckParm("-pak");

        if ((p) &&
            (p < (myargc - 2))
            ) { // create a pak file
            strcpy(srcdir, myargv[p + 1]);
            strcpy(destdir, myargv[p + 2]);
            if (srcdir[strlen(srcdir) - 1] != '/')
                strcat(srcdir, "/");
            DefaultExtension(destdir, ".pak");

            pf = pfiles;
            packhandle = SafeOpenWrite(destdir);
            SafeWrite(packhandle, &header, sizeof(header));
            copytype = 2;
        }
    }
    if (!copytype)
        return;

    char srcfile[1024];
    char destfile[1024];
    for (int i = 0; i < numsounds; i++) {
        if (precache_sounds_block[i] != blocknum)   continue;
        char name[1024]; snprintf(name, sizeof(name), "sound/%s", precache_sounds[i]);
        snprintf(srcfile, sizeof(srcfile), "%s%s", srcdir, name);
        snprintf(destfile, sizeof(destfile), "%s%s", destdir, name);
        if (copytype == 1)  CopyFile(srcfile, destfile);
        else                PackFile(srcfile, name);
    }
    for (int i = 0; i < nummodels; i++) {
        if (precache_models_block[i] != blocknum)   continue;
        snprintf(srcfile, sizeof(srcfile), "%s%s", srcdir, precache_models[i]);
        snprintf(destfile, sizeof(destfile), "%s%s", destdir, precache_models[i]);
        if (copytype == 1)  CopyFile(srcfile, destfile);
        else                PackFile(srcfile, precache_models[i]);
    }
    for (int i = 0; i < numfiles; i++) {
        if (precache_files_block[i] != blocknum)    continue;
        snprintf(srcfile, sizeof(srcfile), "%s%s", srcdir, precache_files[i]);
        snprintf(destfile, sizeof(destfile), "%s%s", destdir, precache_files[i]);
        if (copytype == 1)  CopyFile(srcfile, destfile);
        else                PackFile(srcfile, precache_files[i]);
    }

    if (copytype == 2) {
        header.id[0] = 'P';
        header.id[1] = 'A';
        header.id[2] = 'C';
        header.id[3] = 'K';
        header.dirofs = LittleLong(lseek(packhandle, 0, SEEK_CUR));

        int dirlen = (uint8_p)pf - (uint8_p)pfiles;
        header.dirlen = LittleLong(dirlen);
        SafeWrite(packhandle, pfiles, dirlen);

        lseek(packhandle, 0, SEEK_SET);
        SafeWrite(packhandle, &header, sizeof(header));
        close(packhandle);

        // do a crc of the file
        CRC_t crc;
        CRC_Init(&crc);
        int i = 0;
        for (; i < dirlen; i++)
            CRC_ProcessByte(&crc, ((uint8_p)pfiles)[i]);

        i = pf - pfiles;
        printf(
            "%i files packed in %i bytes (%i crc)\n",
            i, packbytes, crc
        );
    }
}

//============================================================================

/*
============
main
============
*/
int main(int argc, cStr_ar argv) {
    myargc = argc;
    myargv = argv;

    if (CheckParm("-?") ||
        CheckParm("-help")
        ) {
        printf(
            "qcc looks for progs.src in the current directory.\n"
            "to look in a different directory: qcc -src <directory>\n"
            "to build a clean data tree: qcc -copy <srcdir> <destdir>\n"
            "to build a clean pak file: qcc -pak <srcdir> <packfile>\n"
            "to bsp all bmodels: qcc -bspmodels <gamedir>\n"
        );
        return 0;  // return no error
    }

    int p = CheckParm("-src");
    char sourcedir[1024];
    if ((p) &&
        (p < (argc - 1))
        ) {
        strcpy(sourcedir, argv[p + 1]);
        strcat(sourcedir, "/");
        printf("Source directory: %s\n", sourcedir);
    }
    else
        strcpy(sourcedir, "");

    InitData();

    char filename[1024];
    snprintf(filename, sizeof(filename), "%sprogs.src", sourcedir);
    cStr_p src;  LoadFile(filename, (Any_p)&src);

    src = COM_Parse(src);
    if (!src)
        Error("No destination filename.  qcc -help for info.\n");
    strcpy(destfile, com_token);
    printf("outputfile: %s\n", destfile);

    pr_dumpasm = false;
    PR_BeginCompilation(malloc(0x100000), 0x100000);

    // compile all the files
    do {
        src = COM_Parse(src);
        if (!src)
            break;
        snprintf(filename, sizeof(filename), "%s%s", sourcedir, com_token);
        printf("compiling %s\n", filename);
        cStr_p src2; LoadFile(filename, (Any_p)&src2);

        if (!PR_CompileFile(src2, filename))
            exit(1);    // return error

    } while (1);

    if (!PR_FinishCompilation())
        Error("compilation errors");    // return error

    p = CheckParm("-asm");
    if (p)
        for (p++; p < argc; p++) {
            if (argv[p][0] == '-')
                break;
            PrintFunction(argv[p]);
        }

    int crc = PR_WriteProgdefs("progdefs.h");   // write progdefs.h
    WriteData(crc); // write data file
    BspModels();    // regenerate bmodels if -bspmodels
    CopyFiles();    // report / copy the data files

    return 0; // return no error
}
