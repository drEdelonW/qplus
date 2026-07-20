#pragma once

#include "types.h"  // size_t
#include "VM_enumTypes.h"   // etype_t
size_t SizeOfPrType(etype_t type);
#include "VM_enumVal.h" // eval_p
cString PR_ValueString(etype_t type, eval_p val);
cString PR_UglyValueString(etype_t type, eval_p val);