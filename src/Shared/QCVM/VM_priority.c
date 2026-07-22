#include "VM_priority.h"

def_p PR_Term();
bool PR_Check(cStr_p string);
def_p PR_ParseFunctionCall(def_p func);
#include "VM_opcode.h"      // def_p opcode_p
#include "VM_statment.h"    // statements
#include <string.h>         // strcmp
void PR_ParseError(cStr_p error, ...);
/*
==============
PR_Expression
==============
*/
def_p PR_Expression(Priority_t priority) {
    if (priority == PrioTerm)
        return PR_Term();

    def_p e = PR_Expression(priority - 1);
    while (1) {
        if ((priority == PrioAccess) &&
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
                    (type_c != op->type_c->type->type)
                    )
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
