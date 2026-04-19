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

#include "progs.h"
#include "pr_cmds.h"
#include "pr_ops.h"
#include "pr_Statment.h"
#include "pr_Function.h"
#include "console.h"
#include "server.h"
#include "host.h"
#include <string.h>
#include <stdarg.h>

typedef struct {
    int32_t     stack;
    dFunction_p func;
} prstack_t;

#define MAX_STACK_DEPTH  32
static prstack_t _pr_Stack[MAX_STACK_DEPTH];
static int32_t   _pr_Depth;

#define LOCALSTACK_SIZE  2048
static int32_t _localStack[LOCALSTACK_SIZE];
static int32_t _localStack_used;

bool  pr_trace;
static int32_t _pr_xStatement;
int32_t         pr_argc;


//=============================================================================

/*
    =================
    PR_PrintStatement
    =================
*/
void PR_PrintStatement(dStatement_p state) {
    // if ((uint32_t)state->op < (sizeof(_pr_opNames) / sizeof(_pr_opNames[0]))) {
    PR_PrintOperation(state->op);

    if ((state->op == OP_IF) ||
        (state->op == OP_IFNOT)
        )
        Con_Printf("%sbranch %i", PR_GlobalString(state->a), state->b);
    else if (state->op == OP_GOTO)
        Con_Printf("branch %i", state->a);
    else if ((uint32_t)(state->op - OP_STORE_F) < 6) {
        Con_Printf("%s", PR_GlobalString(state->a));
        Con_Printf("%s", PR_GlobalStringNoContents(state->b));
    }
    else {
        if (state->a)   Con_Printf("%s", PR_GlobalString(state->a));
        if (state->b)   Con_Printf("%s", PR_GlobalString(state->b));
        if (state->c)   Con_Printf("%s", PR_GlobalStringNoContents(state->c));
    }
    Con_Printf("\n");
}

/*
    ============
    PR_StackTrace
    ============
*/
void PR_StackTrace() {
    if (_pr_Depth == 0) { Con_Printf("<NO STACK>\n");   return; }

    _pr_Stack[_pr_Depth].func = pr_xFunction;
    for (int i = _pr_Depth; i >= 0; i--) {
        dFunction_p func = _pr_Stack[i].func;

        if (!func)  Con_Printf("<NO FUNCTION>\n");
        else        Con_Printf("%12s : %s\n",
            PR_GetQString(func->s_file),
            PR_GetQString(func->s_name)
        );
    }
}


/*
    ============
    PR_Profile_f

    ============
*/
void PR_Profile_f() {
    dFunction_p best;
    do {
        int max = 0;
        best = NULL;
        for (int i = 0; i < progs->functions.num; i++) {
            dFunction_p func = &pr_functions[i];
            if (func->profile > max) {
                max = func->profile;
                best = func;
            }
        }
        if (best) {
            int num = 0;
            if (num < 10)
                Con_Printf(
                    "%7i %s\n",
                    best->profile,
                    PR_GetQString(best->s_name)
                );
            num++;
            best->profile = 0;
        }
    } while (best);
}


/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError(cString error, ...) {
    va_list argptr;     va_start(argptr, error);
    char string[1024];  vsnprintf(string, sizeof(string), error, argptr);
    va_end(argptr);

    PR_PrintStatement(pr_statements + _pr_xStatement);
    PR_StackTrace();
    Con_Printf("%s\n", string);

    _pr_Depth = 0;  // dump the stack so host_error can shutdown functions

    Host_Error("Program error");
}

/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/

/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
int32_t PR_EnterFunction(dFunction_p func) {
    _pr_Stack[_pr_Depth] = (prstack_t){
        .stack = _pr_xStatement,
        .func = pr_xFunction
    };
    _pr_Depth++;
    if (_pr_Depth >= MAX_STACK_DEPTH)        PR_RunError("stack overflow");

    // save off any locals that the new function steps on
    int param_used = func->locals;
    if (_localStack_used + param_used > LOCALSTACK_SIZE)
        PR_RunError("PR_ExecuteProgram: locals stack overflow\n");

    for (int i = 0; i < param_used; i++)
        _localStack[_localStack_used + i] = ((int32_p)pr_globals)[func->parm_start + i];
    _localStack_used += param_used;

    // copy parameters
    int param_ofs = func->parm_start;
    for (int i = 0; i < func->numparms; i++) {
        for (int j = 0; j < func->parm_size[i]; j++) {
            ((int32_p)pr_globals)[param_ofs] = ((int32_p)pr_globals)[OFS_PARM0 + i * 3 + j];
            param_ofs++;
        }
    }

    pr_xFunction = func;
    return func->first_statement - 1; // offset the state++
}

/*
====================
PR_LeaveFunction
====================
*/
int32_t PR_LeaveFunction() {
    if (_pr_Depth <= 0)     Host_SysError("prog stack underflow");

    // restore locals from the stack
    int32_t param_used = pr_xFunction->locals;
    _localStack_used -= param_used;
    if (_localStack_used < 0)   PR_RunError("PR_ExecuteProgram: locals stack underflow\n");

    for (int i = 0; i < param_used; i++)
        ((int32_t*)pr_globals)[pr_xFunction->parm_start + i] = _localStack[_localStack_used + i];

    // up stack
    _pr_Depth--;
    pr_xFunction = _pr_Stack[_pr_Depth].func;
    return _pr_Stack[_pr_Depth].stack;
}

/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram(func_t fnum) {
    if (!fnum ||
        (fnum >= progs->functions.num)) {
        if (pr_global_struct->self)
            ED_Print(ED_GetEDictByOffs(pr_global_struct->self));
        Host_Error("PR_ExecuteProgram: NULL function");
    }

    dFunction_p func = &pr_functions[fnum];

    int32_t runaway = 100000;
    pr_trace = false;

    // make a stack frame
    int32_t exitdepth = _pr_Depth;

    int32_t stack = PR_EnterFunction(func);

    while (1) {
        stack++; // next statement

        dStatement_p st = &pr_statements[stack];
        eval_p a = (eval_p)&pr_globals[st->a];
        eval_p b = (eval_p)&pr_globals[st->b];
        eval_p c = (eval_p)&pr_globals[st->c];

        if (!--runaway)     PR_RunError("runaway loop error");

        pr_xFunction->profile++;
        _pr_xStatement = stack;

        if (pr_trace)       PR_PrintStatement(st);

        // Con_DPrintf("EXE[%s] a:0x%X b:0x%x\n",
        //     _pr_opNames[st->op],
        //     a->_int,
        //     b->_int
        // );

        switch (st->op) {
            case OP_DONE:
            case OP_RETURN: {
                pr_globals[OFS_RETURN + 0] = pr_globals[st->a + 0];
                pr_globals[OFS_RETURN + 1] = pr_globals[st->a + 1];
                pr_globals[OFS_RETURN + 2] = pr_globals[st->a + 2];

                stack = PR_LeaveFunction();
                if (_pr_Depth == exitdepth)
                    return;  // all done
            } break;

            case OP_MUL_F:      c->_float = a->_float * b->_float;      break;
            case OP_MUL_V: {
                c->_float =
                    a->vector[0] * b->vector[0] +
                    a->vector[1] * b->vector[1] +
                    a->vector[2] * b->vector[2];
            } break;
            case OP_MUL_FV: {
                c->vector[0] = a->_float * b->vector[0];
                c->vector[1] = a->_float * b->vector[1];
                c->vector[2] = a->_float * b->vector[2];
            } break;
            case OP_MUL_VF: {
                c->vector[0] = b->_float * a->vector[0];
                c->vector[1] = b->_float * a->vector[1];
                c->vector[2] = b->_float * a->vector[2];
            } break;

            case OP_DIV_F:      c->_float = a->_float / b->_float;              break;

            case OP_ADD_F:      c->_float = a->_float + b->_float;      break;
            case OP_ADD_V: {
                c->vector[0] = a->vector[0] + b->vector[0];
                c->vector[1] = a->vector[1] + b->vector[1];
                c->vector[2] = a->vector[2] + b->vector[2];
            } break;

            case OP_SUB_F:      c->_float = a->_float - b->_float;      break;
            case OP_SUB_V: {
                c->vector[0] = a->vector[0] - b->vector[0];
                c->vector[1] = a->vector[1] - b->vector[1];
                c->vector[2] = a->vector[2] - b->vector[2];
            } break;

            case OP_EQ_F:       c->_float = a->_float == b->_float;                 break;
            case OP_EQ_V: {
                c->_float =
                    (a->vector[0] == b->vector[0]) &&
                    (a->vector[1] == b->vector[1]) &&
                    (a->vector[2] == b->vector[2]);
            } break;
            case OP_EQ_S:       c->_float = !strcmp(PR_GetQString(a->string), PR_GetQString(b->string));    break;
            case OP_EQ_E:       c->_float = (a->_int == b->_int);                   break;
            case OP_EQ_FNC:     c->_float = a->function == b->function;             break;

            case OP_NE_F:       c->_float = a->_float != b->_float;                 break;
            case OP_NE_V: {
                c->_float =
                    (a->vector[0] != b->vector[0]) ||
                    (a->vector[1] != b->vector[1]) ||
                    (a->vector[2] != b->vector[2]);
            } break;
            case OP_NE_S:       c->_float = (float)strcmp(PR_GetQString(a->string), PR_GetQString(b->string));   break;
            case OP_NE_E:       c->_float = a->_int != b->_int;         break;
            case OP_NE_FNC:     c->_float = a->function != b->function; break;

            case OP_GE:         c->_float = a->_float >= b->_float;     break;
            case OP_LE:         c->_float = a->_float <= b->_float;     break;
            case OP_GT:         c->_float = a->_float > b->_float;      break;
            case OP_LT:         c->_float = a->_float < b->_float;      break;

            //==================

            case OP_LOAD_F:
            case OP_LOAD_S:
            case OP_LOAD_ENT:
            case OP_LOAD_FLD:
            case OP_LOAD_FNC: {
                edict_p ed = ED_GetEDictByOffs(a->edict);
#ifdef PARANOID
                ED_GetEDictIdx(ed);  // make sure it's in range
#endif
                a = (eval_p)((int32_p)&ed->v + b->_int);
                c->_int = a->_int;
            } break;

            case OP_LOAD_V: {
                edict_p ed = ED_GetEDictByOffs(a->edict);
#ifdef PARANOID
                ED_GetEDictIdx(ed);  // make sure it's in range
#endif
                a = (eval_p)((int32_p)&ed->v + b->_int);
                c->vector[0] = a->vector[0];
                c->vector[1] = a->vector[1];
                c->vector[2] = a->vector[2];
            } break;

            case OP_ADDRESS: {
                edict_p ed = ED_GetEDictByOffs(a->edict);
#ifdef PARANOID
                ED_GetEDictIdx(ed);  // make sure it's in range
#endif
                if ((ed == (edict_p)sv.edicts) &&
                    (sv.state == ss_active)
                )
                    PR_RunError("assignment to world entity");

                // c->_int = (uint8_p)((int32_p)&ed->v + b->_int) - (uint8_p)sv.edicts;
                {
                    eval_p ptr = (eval_p)((int32_p)&ed->v + b->_int);
                    c->_int = (int32_t)((uintptr_t)ptr - (uintptr_t)sv.edicts);
                }
            } break;

            case OP_STORE_F:
            case OP_STORE_S:
            case OP_STORE_ENT:
            case OP_STORE_FLD:  // integers
            case OP_STORE_FNC:  b->_int = a->_int;  break;  // pointers
            case OP_STORE_V: {
                b->vector[0] = a->vector[0];
                b->vector[1] = a->vector[1];
                b->vector[2] = a->vector[2];
            } break;

            case OP_STOREP_F:
            case OP_STOREP_S:
            case OP_STOREP_ENT:
            case OP_STOREP_FLD:  // integers
            case OP_STOREP_FNC: {  // pointers
                eval_p ptr = (eval_p)((uint8_p)sv.edicts + b->_int);
                ptr->_int = a->_int;
            } break;
            case OP_STOREP_V: {
                eval_p ptr = (eval_p)((uint8_p)sv.edicts + b->_int);
                ptr->vector[0] = a->vector[0];
                ptr->vector[1] = a->vector[1];
                ptr->vector[2] = a->vector[2];
            } break;

            //==================

            case OP_NOT_F:      c->_float = !a->_float;     break;
            case OP_NOT_V:      c->_float = !a->vector[0] && !a->vector[1] && !a->vector[2];    break;
            case OP_NOT_S:      c->_float = !a->string || !*PR_GetQString(a->string);           break;        // c->_float = !a->string || !pr_strings[a->string];
            case OP_NOT_ENT:    c->_float = (ED_GetEDictByOffs(a->edict) == sv.edicts);             break;
            case OP_NOT_FNC:    c->_float = !a->function;   break;

            case OP_IF: {
                if (a->_int)
                    stack += st->b - 1; // offset the stack++
            } break;
            case OP_IFNOT: {
                if (!a->_int)
                    stack += st->b - 1; // offset the stack++
            } break;

            case OP_CALL0:
            case OP_CALL1:
            case OP_CALL2:
            case OP_CALL3:
            case OP_CALL4:
            case OP_CALL5:
            case OP_CALL6:
            case OP_CALL7:
            case OP_CALL8: {
                pr_argc = st->op - OP_CALL0;
                if (!a->function)
                    PR_RunError("NULL function");

                dFunction_p newf = &pr_functions[a->function];

                if (newf->first_statement < 0) { // negative statements are built in functions
                    int i = -(newf->first_statement);
                    if (i >= pr_numbuiltins)
                        PR_RunError("Bad builtin call number");
                    if (pr_builtins[i] == PF_Fixme)
                        PR_RunError("Not Implimented builtin call number[%d]", i);
                    pr_builtins[i]();
                    break;
                }

                stack = PR_EnterFunction(newf);
            } break;

            case OP_STATE: {
                edict_p ed = ED_GetEDictByOffs(pr_global_struct->self);
                ed->v.nextthink = pr_global_struct->time +
#ifdef FPS_20
                    0.05f;
#else
                    0.1f;
#endif
                if (a->_float != ed->v.frame)
                    ed->v.frame = a->_float;

                ed->v.think = b->function;
            } break;

            case OP_GOTO:   stack += st->a - 1;     break;  // offset the stack++

            case OP_AND:    c->_float = a->_float && b->_float;     break;
            case OP_OR:     c->_float = a->_float || b->_float;     break;

            case OP_BITAND: c->_float = (int)a->_float & (int)b->_float;    break;
            case OP_BITOR:  c->_float = (int)a->_float | (int)b->_float;    break;

            default:    PR_RunError("Bad opcode %i", st->op); break;
        }
    }

}
