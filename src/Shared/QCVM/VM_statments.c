#include "VM_statment.h"

int   numstatements;
dStatement_t statements[MAX_STATEMENTS];
int   statement_linenums[MAX_STATEMENTS];

#include <stdlib.h>
#include <string.h>
#include "VM_statment.h"
#include "VM_prog.h"
#include "VM_opcode.h"   // def_p opcode_p

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
