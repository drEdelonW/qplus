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
#include "GlobVars.h"
#include "pr_cmds.h"
#include "pr_ops.h"
#include "pr_Statement.h"
#include "pr_Function.h"
#include "console.h"
#include "server.h"
#include "host.h"
#include <string.h>
// #include <stdarg.h>
#include "VA.h"


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
        for (int i = 0; i < pProgsDat->functions.num; i++) {
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
    VaBuff_t string;
    VA_EXPAND(string, error);
    PR_PrintStatement(PR_GetStack(_pr_xStatement));
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
        _localStack[_localStack_used + i] = G_INT(func->parm_start + i);
    _localStack_used += param_used;

    // copy parameters
    int param_ofs = func->parm_start;
    for (int i = 0; i < func->numparms; i++) {
        for (int j = 0; j < func->parm_size[i]; j++) {
            G_INT(param_ofs) = G_INT(OFS_PARM0 + i * 3 + j);
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
        G_INT(pr_xFunction->parm_start + i) = _localStack[_localStack_used + i];

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
        (fnum >= pProgsDat->functions.num)) {
        if (GV_pGame()->self)
            ED_Print(ED_GetEDictByOffs(GV_pGame()->self));
        Host_Error("PR_ExecuteProgram: NULL function");
    }
    dFunction_p func = &pr_functions[fnum];
    pr_trace = false;

    int32_t exitdepth = _pr_Depth;    // make a stack frame
    int32_t stack = PR_EnterFunction(func);

    int32_t runaway = 100000;
    while (1) {
        stack++; // next statement

        dStatement_p ST = PR_GetStack(stack);
        eval_p A1 = GV_pEval(ST->a);
        eval_p A2 = GV_pEval(ST->b);
        eval_p R  = GV_pEval(ST->c);

        if (!--runaway)     PR_RunError("runaway loop error");

        pr_xFunction->profile++;
        _pr_xStatement = stack;

        if (pr_trace)   PR_PrintStatement(ST);
#if 0
        Con_DPrintf(
            "EXE[%s] a:0x%X b:0x%x\n",
            _pr_opNames[ST->op],
            A1->_int, A2->_int
        );
#endif

        switch (ST->op) {
            case OP_DONE:
            case OP_RETURN: {
                G_FLOAT(OFS_RETURN + X_AX) = G_FLOAT(ST->a + X_AX);
                G_FLOAT(OFS_RETURN + Y_AX) = G_FLOAT(ST->a + Y_AX);
                G_FLOAT(OFS_RETURN + Z_AX) = G_FLOAT(ST->a + Z_AX);

                stack = PR_LeaveFunction();
                if (_pr_Depth == exitdepth)
                    return;  // all done
            } break;

            case OP_MUL_F:  R->_float = A1->_float * A2->_float;              break;
            case OP_MUL_V:  R->_float = DotProduct(A1->vector, A2->vector);   break;
            case OP_MUL_FV: R->vector = VectorScale(A2->vector, A1->_float);  break;
            case OP_MUL_VF: R->vector = VectorScale(A1->vector, A2->_float);  break;

            case OP_DIV_F:      R->_float = A1->_float / A2->_float;      break;
            case OP_ADD_F:      R->_float = A1->_float + A2->_float;      break;
            case OP_ADD_V: R->vector = VectorAdd(A1->vector, A2->vector); break;

            case OP_SUB_F:      R->_float = A1->_float - A2->_float;      break;
            case OP_SUB_V: R->vector = VectorSubtract(A1->vector, A2->vector); break;

            case OP_EQ_F:       R->_float = A1->_float == A2->_float;     break;
            case OP_EQ_V:       R->_float = VectorCompare(A1->vector, A2->vector);    break;
            case OP_EQ_S:       R->_float = !strcmp(PR_GetQString(A1->string), PR_GetQString(A2->string));        break;
            case OP_EQ_E:       R->_float = (A1->_int == A2->_int);                   break;
            case OP_EQ_FNC:     R->_float = A1->function == A2->function;             break;

            case OP_NE_F:       R->_float = A1->_float != A2->_float;                 break;
            case OP_NE_V:       R->_float = !VectorCompare(A1->vector, A2->vector);   break;
            case OP_NE_S:       R->_float = (float)strcmp(PR_GetQString(A1->string), PR_GetQString(A2->string));  break;
            case OP_NE_E:       R->_float = A1->_int != A2->_int;         break;
            case OP_NE_FNC:     R->_float = A1->function != A2->function; break;

            case OP_GE:         R->_float = A1->_float >= A2->_float;     break;
            case OP_LE:         R->_float = A1->_float <= A2->_float;     break;
            case OP_GT:         R->_float = A1->_float > A2->_float;      break;
            case OP_LT:         R->_float = A1->_float < A2->_float;      break;

            //==================

            case OP_LOAD_F:
            case OP_LOAD_S:
            case OP_LOAD_ENT:
            case OP_LOAD_FLD:
            case OP_LOAD_FNC: {
                edict_p ed = ED_GetEDictByOffs(A1->edict);
#ifdef PARANOID
                ED_GetEDictIdx(ed);  // make sure it's in range
#endif
                R->_int = (A1 = (eval_p)((int32_p)&ed->v + A2->_int))->_int;
            } break;

            case OP_LOAD_V: {
                edict_p ed = ED_GetEDictByOffs(A1->edict);
#ifdef PARANOID
                ED_GetEDictIdx(ed);  // make sure it's in range
#endif
                A1 = (eval_p)((int32_p)&ed->v + A2->_int);
                    R->vector = A1->vector;
            } break;

            case OP_ADDRESS: {
                edict_p ed = ED_GetEDictByOffs(A1->edict);
#ifdef PARANOID
                ED_GetEDictIdx(ed);  // make sure it's in range
#endif
                if ((ed == GetEdictsPtr()) &&
                    (sv.state == ss_active)
                )   PR_RunError("assignment to world entity");

                // R->_int = (uint8_p)((int32_p)&ed->v + A2->_int) - (uint8_p)GetEdictsPtr();
                {
                    eval_p ptr = (eval_p)((int32_p)&ed->v + A2->_int);
                    R->_int = (int32_t)((uintptr_t)ptr - (uintptr_t)GetEdictsPtr());
                }
            } break;

            case OP_STORE_F:
            case OP_STORE_S:
            case OP_STORE_ENT:
            case OP_STORE_FLD:  // integers
            case OP_STORE_FNC:  A2->_int = A1->_int;          break;  // pointers
            case OP_STORE_V:    A2->vector = A1->vector;      break;

            case OP_STOREP_F:
            case OP_STOREP_S:
            case OP_STOREP_ENT:
            case OP_STOREP_FLD:  // integers
            case OP_STOREP_FNC: ((eval_p)((uint8_p)GetEdictsPtr() + A2->_int))->_int = A1->_int;        break;  // pointers
            case OP_STOREP_V:   ((eval_p)((uint8_p)GetEdictsPtr() + A2->_int))->vector = A1->vector;    break;

            //==================

            case OP_NOT_F:      R->_float = !A1->_float;     break;
            case OP_NOT_V:      R->_float = (!A1->vector.x) && (!A1->vector.y) && (!A1->vector.z);  break;
            case OP_NOT_S:      R->_float = (!A1->string) || (!(*PR_GetQString(A1->string)));       break;        // R->_float = !A1->string || !pr_strings[A1->string];
            case OP_NOT_ENT:    R->_float = (ED_GetEDictByOffs(A1->edict) == GetEdictsPtr());       break;
            case OP_NOT_FNC:    R->_float = !A1->function;   break;

            case OP_IF: {
                if (A1->_int)
                    stack += ST->b - 1; // offset the stack++
            } break;
            case OP_IFNOT: {
                if (!A1->_int)
                    stack += ST->b - 1; // offset the stack++
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
                pr_argc = ST->op - OP_CALL0;
                if (!A1->function)
                    PR_RunError("NULL function");

                dFunction_p newf = &pr_functions[A1->function];

                if (newf->first_statement < 0) { // negative statements are built in functions
                    int i = -(newf->first_statement);
                    if (i >= pr_numbuiltins)        PR_RunError("Bad builtin call number");
                    if (pr_builtins[i] == PF_Fixme) PR_RunError("Not Implimented builtin call number[%d]", i);

                    pr_builtins[i]();
                    break;
                }

                stack = PR_EnterFunction(newf);
            } break;

            case OP_STATE: {
                edict_p ed = ED_GetEDictByOffs(GV_pGame()->self);
                ed->v.nextthink = GV_pGame()->time +
#ifdef FPS_20
                    0.05f;
#else
                    0.1f;
#endif
                if (A1->_float != ed->v.frame)
                    ed->v.frame = A1->_float;

                ed->v.think = A2->function;
            } break;

            case OP_GOTO:   stack += ST->a - 1;     break;  // offset the stack++

            case OP_AND:    R->_float = A1->_float && A2->_float;     break;
            case OP_OR:     R->_float = A1->_float || A2->_float;     break;

            case OP_BITAND: R->_float = (int)A1->_float & (int)A2->_float;    break;
            case OP_BITOR:  R->_float = (int)A1->_float | (int)A2->_float;    break;

            default:    PR_RunError("Bad opcode %i", ST->op); break;
        }
    }

}
