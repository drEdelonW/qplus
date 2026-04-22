#include "pr_Statement.h"
#include "endian_tools.h"

dStatement_p   pr_statements;


void initProgStatement(TypeLess_ptr base, progLump_t pl) {
    pr_statements = (dStatement_p)((uint8_p)base + pl.ofs);
    // uint8_t swap the lumps
    for (int i = 0; i < pl.num; i++) {
        pr_statements[i].op = (op_type)LittleShort((int16_t)pr_statements[i].op);
        pr_statements[i].a = LittleShort(pr_statements[i].a);
        pr_statements[i].b = LittleShort(pr_statements[i].b);
        pr_statements[i].c = LittleShort(pr_statements[i].c);
    }
}