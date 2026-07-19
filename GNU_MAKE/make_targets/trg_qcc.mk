DST_PLATFORM := POSIX

$(eval QCC_DIR = ../src/qcc) $(eval INCLUDES += $(QCC_DIR))
	SRC_LIST += $(QCC_DIR)/cmdlib.c
	SRC_LIST += $(QCC_DIR)/pr_lex.c
	SRC_LIST += $(QCC_DIR)/pr_comp.c
	SRC_LIST += $(QCC_DIR)/qcc.c

    $(eval QCVM_DIR = $(QCC_DIR)/QCVM) $(eval INCLUDES += $(QCVM_DIR))
        SRC_LIST += $(QCVM_DIR)/VM_types.c

  $(eval MATH_DIR = ../src/engine/Shared/math) $(eval INCLUDES += $(MATH_DIR))
  $(eval CUTILS_DIR = ../src/engine/Shared/utils) $(eval INCLUDES += $(CUTILS_DIR))

vpath %.c $(QCC_DIR) $(CUTILS_DIR)

# NOTE: builtin.c is NOT part of qcc — it's the original engine's server-side
# PF_* builtins implementation (#include "quakedef.h"), bundled into the same
# archive but never referenced by qcc.c/qcc.h. Left out on purpose.
