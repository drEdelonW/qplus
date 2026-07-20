#pragma once

#include "types.h"
#include "VM_operation.h"   // prog_operation_t

void PR_PrintOperation(prog_operation_t op);
cStr_p PR_GlobalString(int32_t ofs);
cStr_p PR_GlobalStringNoContents(int32_t ofs);
