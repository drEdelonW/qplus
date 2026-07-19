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

pr_info_t pr;
def_p pr_global_defs[MAX_REGS]; // to find def for a global variable
int   pr_edict_size;

//========================================

def_p pr_scope;  // the function being parsed, or NULL
bool pr_dumpasm;
string_t s_file;   // filename for function definition
int   locals_end;  // for tracking local variables vs temps
jmp_buf  pr_parse_abort;  // longjump with this on parse error
void PR_ParseDefs();

//========================================


opcode_t pr_opcodes[] = {
    [OP_DONE] = {"<DONE>", "DONE", -1, false, &def_entity, &def_field, &def_void},

    [OP_MUL_F] = {"*", "MUL_F", 2, false, &def_float, &def_float, &def_float},
    [OP_MUL_V] = {"*", "MUL_V", 2, false, &def_vector, &def_vector, &def_float},
    [OP_MUL_FV] = {"*", "MUL_FV", 2, false, &def_float, &def_vector, &def_vector},
    [OP_MUL_VF] = {"*", "MUL_VF", 2, false, &def_vector, &def_float, &def_vector},

    [OP_DIV_F] = {"/", "DIV", 2, false, &def_float, &def_float, &def_float},

    [OP_ADD_F] = {"+", "ADD_F", 3, false, &def_float, &def_float, &def_float},
    [OP_ADD_V] = {"+", "ADD_V", 3, false, &def_vector, &def_vector, &def_vector},

    [OP_SUB_F] = {"-", "SUB_F", 3, false, &def_float, &def_float, &def_float},
    [OP_SUB_V] = {"-", "SUB_V", 3, false, &def_vector, &def_vector, &def_vector},

    [OP_EQ_F] = {"==", "EQ_F", 4, false, &def_float, &def_float, &def_float},
    [OP_EQ_V] = {"==", "EQ_V", 4, false, &def_vector, &def_vector, &def_float},
    [OP_EQ_S] = {"==", "EQ_S", 4, false, &def_string, &def_string, &def_float},
    [OP_EQ_E] = {"==", "EQ_E", 4, false, &def_entity, &def_entity, &def_float},
    [OP_EQ_FNC] = {"==", "EQ_FNC", 4, false, &def_function, &def_function, &def_float},

    [OP_NE_F] = {"!=", "NE_F", 4, false, &def_float, &def_float, &def_float},
    [OP_NE_V] = {"!=", "NE_V", 4, false, &def_vector, &def_vector, &def_float},
    [OP_NE_S] = {"!=", "NE_S", 4, false, &def_string, &def_string, &def_float},
    [OP_NE_E] = {"!=", "NE_E", 4, false, &def_entity, &def_entity, &def_float},
    [OP_NE_FNC] = {"!=", "NE_FNC", 4, false, &def_function, &def_function, &def_float},

    [OP_LE] = {"<=", "LE", 4, false, &def_float, &def_float, &def_float},
    [OP_GE] = {">=", "GE", 4, false, &def_float, &def_float, &def_float},
    [OP_LT] = {"<", "LT", 4, false, &def_float, &def_float, &def_float},
    [OP_GT] = {">", "GT", 4, false, &def_float, &def_float, &def_float},

    [OP_LOAD_F] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_float},
    [OP_LOAD_V] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_vector},
    [OP_LOAD_S] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_string},
    [OP_LOAD_ENT] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_entity},
    [OP_LOAD_FLD] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_field},
    [OP_LOAD_FNC] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_function},

    [OP_ADDRESS] = {".", "ADDRESS", 1, false, &def_entity, &def_field, &def_pointer},

    [OP_STORE_F] = {"=", "STORE_F", 5, true, &def_float, &def_float, &def_float},
    [OP_STORE_V] = {"=", "STORE_V", 5, true, &def_vector, &def_vector, &def_vector},
    [OP_STORE_S] = {"=", "STORE_S", 5, true, &def_string, &def_string, &def_string},
    [OP_STORE_ENT] = {"=", "STORE_ENT", 5, true, &def_entity, &def_entity, &def_entity},
    [OP_STORE_FLD] = {"=", "STORE_FLD", 5, true, &def_field, &def_field, &def_field},
    [OP_STORE_FNC] = {"=", "STORE_FNC", 5, true, &def_function, &def_function, &def_function},

    [OP_STOREP_F] = {"=", "STOREP_F", 5, true, &def_pointer, &def_float, &def_float},
    [OP_STOREP_V] = {"=", "STOREP_V", 5, true, &def_pointer, &def_vector, &def_vector},
    [OP_STOREP_S] = {"=", "STOREP_S", 5, true, &def_pointer, &def_string, &def_string},
    [OP_STOREP_ENT] = {"=", "STOREP_ENT", 5, true, &def_pointer, &def_entity, &def_entity},
    [OP_STOREP_FLD] = {"=", "STOREP_FLD", 5, true, &def_pointer, &def_field, &def_field},
    [OP_STOREP_FNC] = {"=", "STOREP_FNC", 5, true, &def_pointer, &def_function, &def_function},

    [OP_RETURN] = {"<RETURN>", "RETURN", -1, false, &def_void, &def_void, &def_void},

    [OP_NOT_F] = {"!", "NOT_F", -1, false, &def_float, &def_void, &def_float},
    [OP_NOT_V] = {"!", "NOT_V", -1, false, &def_vector, &def_void, &def_float},
    [OP_NOT_S] = {"!", "NOT_S", -1, false, &def_vector, &def_void, &def_float},
    [OP_NOT_ENT] = {"!", "NOT_ENT", -1, false, &def_entity, &def_void, &def_float},
    [OP_NOT_FNC] = {"!", "NOT_FNC", -1, false, &def_function, &def_void, &def_float},

    [OP_IF] = {"<IF>", "IF", -1, false, &def_float, &def_float, &def_void},
    [OP_IFNOT] = {"<IFNOT>", "IFNOT", -1, false, &def_float, &def_float, &def_void},

    // calls returns REG_RETURN
    [OP_CALL0] = {"<CALL0>", "CALL0", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL1] = {"<CALL1>", "CALL1", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL2] = {"<CALL2>", "CALL2", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL3] = {"<CALL3>", "CALL3", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL4] = {"<CALL4>", "CALL4", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL5] = {"<CALL5>", "CALL5", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL6] = {"<CALL6>", "CALL6", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL7] = {"<CALL7>", "CALL7", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL8] = {"<CALL8>", "CALL8", -1, false, &def_function, &def_void, &def_void},

    [OP_STATE] = {"<STATE>", "STATE", -1, false, &def_float, &def_float, &def_void},

    [OP_GOTO] = {"<GOTO>", "GOTO", -1, false, &def_float, &def_void, &def_void},

    [OP_AND] = {"&&", "AND", 6, false, &def_float, &def_float, &def_float},
    [OP_OR] = {"||", "OR", 6, false, &def_float, &def_float, &def_float},

    [OP_BITAND] = {"&", "BITAND", 2, false, &def_float, &def_float, &def_float},
    [OP_BITOR] = {"|", "BITOR", 2, false, &def_float, &def_float, &def_float},

    [OP_LAST] = {NULL}
};

#define TOP_PRIORITY 6
#define NOT_PRIORITY 4

def_p PR_Expression(int priority);

def_t junkdef;

//===========================================================================


/*
============
PR_Statement

Emits a primitive statement, returning the var it places it's value in
============
*/
def_p PR_Statement(opcode_p op, def_p var_a, def_p var_b) {
    dStatement_p statement = &statements[numstatements];
    numstatements++;

    statement_linenums[statement - statements] = pr_source_line;
    statement->op = op - pr_opcodes;
    statement->a = (var_a) ? var_a->ofs : 0;
    statement->b = (var_b) ? var_b->ofs : 0;
    def_p var_c;
    if ((op->type_c == &def_void) ||
        (op->right_associative)
        ) {
        var_c = NULL;
        statement->c = 0;   // ifs, gotos, and assignments
        // don't need vars allocated
    }
    else { // allocate result space
        var_c = malloc(sizeof(def_t));
        memset(var_c, 0, sizeof(def_t));
        var_c->ofs = numpr_globals;
        var_c->type = op->type_c->type;

        statement->c = numpr_globals;
        numpr_globals += type_size[op->type_c->type->type];
    }

    if (op->right_associative)
        return var_a;
    return var_c;
}

/*
============
PR_ParseImmediate

Looks for a preexisting constant
============
*/
def_p PR_ParseImmediate() {
    // check for a constant with the same value
    def_p cn = pr.def_head.next;
    for (; cn; cn = cn->next) {
        if (!(cn->initialized) ||
            (cn->type != pr_immediate_type)
            )   continue;

        /**/ if (pr_immediate_type == &type_string) {
            if (!strcmp(G_STRING(cn->ofs), pr_immediate_string)) {
                PR_Lex();
                return cn;
            }
        }
        else if (pr_immediate_type == &type_float) {
            if (G_FLOAT(cn->ofs) == pr_immediate._float) {
                PR_Lex();
                return cn;
            }
        }
        else if (pr_immediate_type == &type_vector) {
            if ((G_FLOAT(cn->ofs + 0) == pr_immediate.vector[0]) &&
                (G_FLOAT(cn->ofs + 1) == pr_immediate.vector[1]) &&
                (G_FLOAT(cn->ofs + 2) == pr_immediate.vector[2])
                ) {
                PR_Lex();
                return cn;
            }
        }
        else
            PR_ParseError("weird immediate type");
    }

    // allocate a new one
    cn = malloc(sizeof(def_t));
    cn->next = NULL;
    pr.def_tail->next = cn;
    pr.def_tail = cn;
    cn->type = pr_immediate_type;
    cn->name = "IMMEDIATE";
    cn->initialized = 1;
    cn->scope = NULL;  // always share immediates

    // copy the immediate to the global area
    cn->ofs = numpr_globals;
    pr_global_defs[cn->ofs] = cn;
    numpr_globals += type_size[pr_immediate_type->type];
    if (pr_immediate_type == &type_string)
        pr_immediate.string = CopyString(pr_immediate_string);

    memcpy(pr_globals + cn->ofs, &pr_immediate, MUL4(type_size[pr_immediate_type->type]));

    PR_Lex();

    return cn;
}


void PrecacheSound(def_p e, int ch) {
    if (!e->ofs)
        return;

    cStr_p n = G_STRING(e->ofs);
    int i = 0;
    for (i = 0; i < numsounds; i++)
        if (!strcmp(n, precache_sounds[i]))
            return;
    if (numsounds == MAX_SOUNDS)
        Error("PrecacheSound: numsounds == MAX_SOUNDS");
    strcpy(precache_models[i], n);
    if ((ch >= '1') && (ch <= '9'))     precache_models_block[i] = ch - '0';
    else                                precache_models_block[i] = 1;
    numsounds++;
}

void PrecacheModel(def_p e, int ch) {
    if (!e->ofs)
        return;

    cStr_p n = G_STRING(e->ofs);
    int i = 0;
    for (; i < nummodels; i++)
        if (!strcmp(n, precache_models[i]))
            return;
    if (numsounds == MAX_SOUNDS)
        Error("PrecacheModels: numsounds == MAX_SOUNDS");
    strcpy(precache_models[i], n);
    if ((ch >= '1') && (ch <= '9'))     precache_models_block[i] = ch - '0';
    else                                precache_models_block[i] = 1;
    nummodels++;
}

void PrecacheFile(def_p e, int ch) {
    if (!e->ofs)
        return;

    cStr_p n = G_STRING(e->ofs);
    int  i = 0;
    for (; i < numfiles; i++)
        if (!strcmp(n, precache_files[i]))
            return;
    if (numfiles == MAX_FILES)
        Error("PrecacheFile: numfiles == MAX_FILES");
    strcpy(precache_files[i], n);
    if ((ch >= '1') && (ch <= '9'))     precache_files_block[i] = ch - '0';
    else                                precache_files_block[i] = 1;
    numfiles++;
}

/*
============
PR_ParseFunctionCall
============
*/
def_p PR_ParseFunctionCall(def_p func) {
    type_p fnTyp = func->type;
    if (fnTyp->type != ev_function)
        PR_ParseError("not a function");

    // copy the arguments to the global parameter variables
    int arg = 0;
    if (!PR_Check(")")) {
        do {
            if ((fnTyp->num_parms != -1) &&
                (fnTyp->num_parms <= arg)
                )   PR_ParseError("too many parameters");
            def_p e = PR_Expression(TOP_PRIORITY);

            if ((arg == 0) &&
                (func->name)
                ) {
                // save information for model and sound caching
                /**/ if (!strncmp(func->name, "precache_sound", 14))    PrecacheSound(e, func->name[14]);
                else if (!strncmp(func->name, "precache_model", 14))    PrecacheModel(e, func->name[14]);
                else if (!strncmp(func->name, "precache_file", 13))     PrecacheFile(e, func->name[13]);
            }

            if ((fnTyp->num_parms != -1) &&
                (fnTyp->parm_types[arg] != e->type)
                )   PR_ParseError("type mismatch on parm %i", arg);
            // a vector copy will copy everything
            def_parms[arg].type = fnTyp->parm_types[arg];
            PR_Statement(&pr_opcodes[OP_STORE_V], e, &def_parms[arg]);
            arg++;
        } while (PR_Check(","));

        if ((fnTyp->num_parms != -1) &&
            (fnTyp->num_parms != arg)
            )   PR_ParseError("too few parameters");
        PR_Expect(")");
    }
    if (arg > 8)
        PR_ParseError("More than eight parameters");

    PR_Statement(&pr_opcodes[OP_CALL0 + arg], func, 0);

    def_ret.type = fnTyp->aux_type;
    return &def_ret;
}

/*
============
PR_ParseValue

Returns the global ofs for the current token
============
*/
def_p PR_ParseValue() {
    if (pr_token_type == tt_immediate)  // if the token is an immediate, allocate a constant for it
        return PR_ParseImmediate();

    // look through the defs
    cStr_p name = PR_ParseName();
    def_p d = PR_GetDef(NULL, name, pr_scope, false);
    if (!d)
        PR_ParseError("Unknown value \"%s\"", name);
    return d;
}


/*
============
PR_Term
============
*/
def_p PR_Term() {
    if (PR_Check("!")) {
        def_p e = PR_Expression(NOT_PRIORITY);
        etype_t t = e->type->type;

        def_p e2;
        /**/ if (t == ev_float)     e2 = PR_Statement(&pr_opcodes[OP_NOT_F], e, 0);
        else if (t == ev_string)    e2 = PR_Statement(&pr_opcodes[OP_NOT_S], e, 0);
        else if (t == ev_entity)    e2 = PR_Statement(&pr_opcodes[OP_NOT_ENT], e, 0);
        else if (t == ev_vector)    e2 = PR_Statement(&pr_opcodes[OP_NOT_V], e, 0);
        else if (t == ev_function)  e2 = PR_Statement(&pr_opcodes[OP_NOT_FNC], e, 0);
        else {
            e2 = NULL;  // shut up compiler warning;
            PR_ParseError("type mismatch for !");
        }
        return e2;
    }

    if (PR_Check("(")) {
        def_p e = PR_Expression(TOP_PRIORITY);
        PR_Expect(")");
        return e;
    }

    return PR_ParseValue();
}

/*
==============
PR_Expression
==============
*/

def_p PR_Expression(int priority) {
    if (priority == 0)
        return PR_Term();

    def_p e = PR_Expression(priority - 1);
    while (1) {
        if ((priority == 1) &&
            (PR_Check("("))
            )   return PR_ParseFunctionCall(e);

        opcode_p op;
        for (op = pr_opcodes; op->name; op++) {
            if ((op->priority != priority) ||
                (!PR_Check(op->name))
                )   continue;

            def_p e2;
            if (op->right_associative) {
                // if last statement is an indirect, change it to an address of
                if ((unsigned)(statements[numstatements - 1].op - OP_LOAD_F) < 6) {
                    statements[numstatements - 1].op = OP_ADDRESS;
                    def_pointer.type->aux_type = e->type;
                    e->type = def_pointer.type;
                }
                e2 = PR_Expression(priority);
            }
            else
                e2 = PR_Expression(priority - 1);

            // type check
            etype_t type_a = e->type->type;
            etype_t type_b = e2->type->type;
            etype_t type_c;
            if (op->name[0] == '.') {// field access gets type from field
                if (e2->type->aux_type)     type_c = e2->type->aux_type->type;
                else                        type_c = -1; // not a field
            }
            else
                type_c = ev_void;

            opcode_p oldop = op;
            while (
                (type_a != op->type_a->type->type) ||
                (type_b != op->type_b->type->type) ||
                (
                    (type_c != ev_void) &&
                    (type_c != op->type_c->type->type))
                ) {
                op++;
                if (!(op->name) ||
                    (strcmp(op->name, oldop->name))
                    )   PR_ParseError("type mismatch for %s", oldop->name);
            }

            if ((type_a == ev_pointer) &&
                (type_b != e->type->aux_type->type)
                )   PR_ParseError("type mismatch for %s", op->name);


            if (op->right_associative)  e = PR_Statement(op, e2, e);
            else                        e = PR_Statement(op, e, e2);

            if (type_c != ev_void) // field access gets type from field
                e->type = e2->type->aux_type;

            break;
        }
        if (!op->name)
            break; // next token isn't at this priority level
    }

    return e;
}


/*
============
PR_ParseStatement

============
*/
void PR_ParseStatement() {
    if (PR_Check("{")) {
        do {
            PR_ParseStatement();
        } while (!PR_Check("}"));
        return;
    }

    if (PR_Check("return")) {
        if (PR_Check(";")) {
            PR_Statement(&pr_opcodes[OP_RETURN], 0, 0);
            return;
        }
        def_p e = PR_Expression(TOP_PRIORITY);
        PR_Expect(";");
        PR_Statement(&pr_opcodes[OP_RETURN], e, 0);
        return;
    }

    if (PR_Check("while")) {
        PR_Expect("(");
        dStatement_p patch2 = &statements[numstatements];
        def_p e = PR_Expression(TOP_PRIORITY);
        PR_Expect(")");
        dStatement_p patch1 = &statements[numstatements];
        PR_Statement(&pr_opcodes[OP_IFNOT], e, 0);
        PR_ParseStatement();
        junkdef.ofs = patch2 - &statements[numstatements];
        PR_Statement(&pr_opcodes[OP_GOTO], &junkdef, 0);
        patch1->b = &statements[numstatements] - patch1;
        return;
    }

    if (PR_Check("do")) {
        dStatement_p patch1 = &statements[numstatements];
        PR_ParseStatement();
        PR_Expect("while");
        PR_Expect("(");
        def_p e = PR_Expression(TOP_PRIORITY);
        PR_Expect(")");
        PR_Expect(";");
        junkdef.ofs = patch1 - &statements[numstatements];
        PR_Statement(&pr_opcodes[OP_IF], e, &junkdef);
        return;
    }

    if (PR_Check("local")) {
        PR_ParseDefs();
        locals_end = numpr_globals;
        return;
    }

    if (PR_Check("if")) {
        PR_Expect("(");
        def_p e = PR_Expression(TOP_PRIORITY);
        PR_Expect(")");

        dStatement_p patch1 = &statements[numstatements];
        PR_Statement(&pr_opcodes[OP_IFNOT], e, 0);

        PR_ParseStatement();

        if (PR_Check("else")) {
            dStatement_p patch2 = &statements[numstatements];
            PR_Statement(&pr_opcodes[OP_GOTO], 0, 0);
            patch1->b = &statements[numstatements] - patch1;
            PR_ParseStatement();
            patch2->a = &statements[numstatements] - patch2;
        }
        else
            patch1->b = &statements[numstatements] - patch1;

        return;
    }

    PR_Expression(TOP_PRIORITY);
    PR_Expect(";");
}


/*
==============
PR_ParseState

States are special functions made for convenience.  They automatically
set frame, nextthink (implicitly), and think (allowing forward definitions).

// void() name = [framenum, nextthink] {code}
// expands to:
// function void name ()
// {
//  self.frame=framenum;
//  self.nextthink = time + 0.1;
//  self.think = nextthink
//  <code>
// };
==============
*/
void PR_ParseState() {
    if ((pr_token_type != tt_immediate) ||
        (pr_immediate_type != &type_float)
        )   PR_ParseError("state frame must be a number");
    def_p s1 = PR_ParseImmediate();

    PR_Expect(",");

    cStr_p name = PR_ParseName();
    def_p def = PR_GetDef(&type_function, name, 0, true);

    PR_Expect("]");

    PR_Statement(&pr_opcodes[OP_STATE], s1, def);
}

/*
============
PR_ParseImmediateStatements

Parse a function body
============
*/
function_p PR_ParseImmediateStatements(type_p type) {
    // check for builtin function definition #1, #2, etc
    function_p f = malloc(sizeof(function_t));
    if (PR_Check("#")) {
        if ((pr_token_type != tt_immediate) ||
            (pr_immediate_type != &type_float) ||
            (pr_immediate._float != (int)pr_immediate._float)
            )   PR_ParseError("Bad builtin immediate");

        f->builtin = (int)pr_immediate._float;
        PR_Lex();
        return f;
    }

    f->builtin = 0;
    // define the parms
    for (int i = 0; i < type->num_parms; i++) {
        f->parm_ofs[i] = PR_GetDef(
            type->parm_types[i],
            pr_parm_names[i],
            pr_scope, true
        )->ofs;
        if ((i > 0) &&
            (f->parm_ofs[i] < f->parm_ofs[i - 1])
            )   Error("bad parm order");
    }

    f->code = numstatements;

    //
    // check for a state opcode
    //
    if (PR_Check("["))
        PR_ParseState();

    //
    // parse regular statements
    //
    PR_Expect("{");

    while (!PR_Check("}"))
        PR_ParseStatement();

    // emit an end of statements opcode
    PR_Statement(pr_opcodes, 0, 0);


    return f;
}

/*
============
PR_GetDef

If type is NULL, it will match any type
If allocate is true, a new def will be allocated if it can't be found
============
*/
def_p PR_GetDef(type_p type, cStr_p name, def_p scope, bool allocate) {
    // see if the name is already in use
    for (def_p def = pr.def_head.next; def; def = def->next)
        if (!strcmp(def->name, name)) {
            if ((def->scope) &&
                (def->scope != scope)
                )   continue;  // in a different function

            if (type && def->type != type)
                PR_ParseError("Type mismatch on redeclaration of %s", name);
            return def;
        }

    if (!allocate)
        return NULL;

    // allocate a new def
    def_p def = malloc(sizeof(def_t));
    memset(def, 0, sizeof(*def));
    def->next = NULL;
    pr.def_tail->next = def;
    pr.def_tail = def;

    def->name = malloc(strlen(name) + 1);
    strcpy(def->name, name);
    def->type = type;

    def->scope = scope;

    def->ofs = numpr_globals;
    pr_global_defs[numpr_globals] = def;

    //
    // make automatic defs for the vectors elements
    // .origin can be accessed as .origin_x, .origin_y, and .origin_z
    //
    char element[MAX_NAME];
    if (type->type == ev_vector) {
        sprintf(element, "%s_x", name);        PR_GetDef(&type_float, element, scope, true);
        sprintf(element, "%s_y", name);        PR_GetDef(&type_float, element, scope, true);
        sprintf(element, "%s_z", name);        PR_GetDef(&type_float, element, scope, true);
    }
    else
        numpr_globals += type_size[type->type];

    if (type->type == ev_field) {
        *(int*)&pr_globals[def->ofs] = pr.size_fields;

        if (type->aux_type->type == ev_vector) {
            sprintf(element, "%s_x", name);            PR_GetDef(&type_floatfield, element, scope, true);
            sprintf(element, "%s_y", name);            PR_GetDef(&type_floatfield, element, scope, true);
            sprintf(element, "%s_z", name);            PR_GetDef(&type_floatfield, element, scope, true);
        }
        else
            pr.size_fields += type_size[type->aux_type->type];
    }

    // if (pr_dumpasm)
    //  PR_PrintOfs (def->ofs);

    return def;
}

/*
================
PR_ParseDefs

Called at the outer layer and when a local statement is hit
================
*/
void PR_ParseDefs() {
    type_p type = PR_ParseType();

    if (pr_scope &&
        (
            (type->type == ev_field) ||
            (type->type == ev_function)
            )
        )   PR_ParseError("Fields and functions must be global");

    do {
        cStr_p name = PR_ParseName();

        def_p def = PR_GetDef(type, name, pr_scope, true);

        // check for an initialization
        if (PR_Check("=")) {
            if (def->initialized)
                PR_ParseError("%s redeclared", name);

            if (type->type == ev_function) {
                int locals_start = locals_end = numpr_globals;
                pr_scope = def;
                function_p f = PR_ParseImmediateStatements(type);
                pr_scope = NULL;
                def->initialized = 1;
                G_FUNCTION(def->ofs) = numfunctions;
                f->def = def;
                //    if (pr_dumpasm)
                //     PR_PrintFunction (def);

                        // fill in the dfunction
                dfunction_p df = &functions[numfunctions];
                numfunctions++;
                if (f->builtin)     df->first_statement = -f->builtin;
                else                df->first_statement = f->code;
                df->s_name = CopyString(f->def->name);
                df->s_file = s_file;
                df->numparms = f->def->type->num_parms;
                df->locals = locals_end - locals_start;
                df->parm_start = locals_start;
                for (int i = 0; i < df->numparms; i++)
                    df->parm_size[i] = type_size[f->def->type->parm_types[i]->type];

                continue;
            }
            else if (pr_immediate_type != type)
                PR_ParseError("wrong immediate type for %s", name);

            def->initialized = 1;
            memcpy(pr_globals + def->ofs, &pr_immediate, (4 * type_size[pr_immediate_type->type]));
            PR_Lex();
        }

    } while (PR_Check(","));

    PR_Expect(";");
}

/*
============
PR_CompileFile

compiles the 0 terminated text, adding defintions to the pr structure
============
*/
bool PR_CompileFile(cStr_p string, cStr_p filename) {
    if (!pr.memory)
        Error("PR_CompileFile: Didn't clear");

    PR_ClearGrabMacros(); // clear the frame macros

    pr_file_p = string;
    s_file = CopyString(filename);

    pr_source_line = 0;

    PR_NewLine();

    PR_Lex(); // read first token

    while (pr_token_type != tt_eof) {
        if (setjmp(pr_parse_abort)) {
            if (++pr_error_count > MAX_ERRORS)
                return false;
            PR_SkipToSemicolon();
            if (pr_token_type == tt_eof)
                return false;
        }

        pr_scope = NULL; // outside all functions

        PR_ParseDefs();
    }

    return (pr_error_count == 0);
}

