#pragma once

#include "pr_cmds.h"
#include "types.h"

typedef void (*builtin_t)();
extern builtin_t*   pr_builtins;
extern int32_t      pr_numbuiltins;

void PF_Fixme();
