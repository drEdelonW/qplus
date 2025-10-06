# TARGET ?= quakeX11
TARGET ?= quakeWin
# TARGET ?= quakeSTM32
# TARGET ?= testSTM32


# ---------- Paths ----------
SRC_DIR      := ../src
IDSRC_DIR    := ../ID_src/WinQuake
STMSRC_DIR   := ../MCU_src

OBJ_DIR      ?= ./objs/$(TARGET)
OUT_DIR      ?= OUTPUT/$(TARGET)

# ---------- Build options ----------
# DEBUG=1 for -O0 -g; DEBUG=0 for -O2 -DNDEBUG
DEBUG        ?= 1
# Set to 1 if you want 32-bit build on Linux x86_64 (requires multilibs)
FORCE_32     ?= 1
# Disable legacy ASM paths
NO_ASM       ?= 1


UNAME_S      := $(shell uname -s)
UNAME_M      := $(shell uname -m)
# Populated by SRCS.mk: must define SRC_LIST and optionally INCLUDES
INCLUDES     :=
DEFINES      :=
SRC_LIST     :=


TRG_MK_FILE := make_targets/trg_$(TARGET).mk
ifeq ($(wildcard $(TRG_MK_FILE)),)
  $(info File [$(TRG_MK_FILE)] not found, skipping include)
  include make_targets/SRCS.mk
else
  $(info File [$(TRG_MK_FILE)] found, including)
  include $(TRG_MK_FILE)
endif

ECHO = echo

# --- order-preserving map SRC -> OBJ ---
# helper: change .c/.cpp -> .o
to_o   = $(patsubst %.cpp,%.o, \
			$(patsubst %.c,%.o, \
				$(patsubst %.s,%.o, \
					$(patsubst %.S,%.o, \
					$1 ))))

# helper: map full source path to proper object path
map_obj = $(if $(filter $(SRC_DIR)/%,$1), \
             $(OBJ_DIR)/$(patsubst $(SRC_DIR)/%,%,$(call to_o,$1)), \
             $(if $(filter $(IDSRC_DIR)/%,$1), \
                 $(OBJ_DIR)/$(patsubst $(IDSRC_DIR)/%,%,$(call to_o,$1)), \
                 $(if $(filter $(STMSRC_DIR)/%,$1), \
                     $(OBJ_DIR)/$(patsubst $(STMSRC_DIR)/%,%,$(call to_o,$1)), \
                     $(OBJ_DIR)/$(notdir $(call to_o,$1)) )) )

OBJS := $(foreach f,$(SRC_LIST),$(call map_obj,$(f)))

# ---------- Toolchain ----------
CPPFLAGS    := $(addprefix -I,$(INCLUDES))
CPPFLAGS    += $(addprefix -D,$(DEFINES))
include GCC_tools_$(DST_PLATFORM).mk


ifneq ($(NO_PARALLEL),1)
  J ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || \
                nproc 2>/dev/null || \
                sysctl -n hw.ncpu 2>/dev/null || echo 1)
#   MAKEFLAGS += -j4
# 	$(J) --output-sync=target
endif

# ---------- Targets ----------
TARGETS      ?= $(OUT_DIR)/$(TARGET)

.PHONY: all build targets clean fresh run rund
.NOTPARALLEL: fresh clean build

all: $(TARGETS)
build: $(TARGETS)
targets: $(TARGETS)

# Main link (use LDFLAGS/LDLIBS properly)
$(TARGETS): $(OBJS) | $(OUT_DIR)
	@mkdir -p $(dir $@)
	@$(LD) $(LDFLAGS) -o $@.elf $^ $(LDLIBS)
	@cd run_env && ln -sf ../$(TARGETS).elf quake
	@$(info DONE.)

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)


include cli_tool.mk

# ---------- Compile rules ----------
# Note: use CPPFLAGS for -D/-I, CFLAGS for codegen; generate .d files

# Canonical reusable recipe
define compile_template
	@printf "$(BLD)$(1)$(RST)[%-45s] " "$<"
	@mkdir -p $(dir $@)
	@{ out=$$($(TIME) \
				$(2) $(CPPFLAGS) $(3) -c $< -o $@ 2>&1); \
		rc=$$?; if [ $$rc -eq 0 ]; then st=$(GRN)OK$(RST); else st="$(RED)FAIL$(RST)\n"; fi; \
		$(ECHO) "$$st  $$out"; exit $$rc;}
endef

COMPILE_C   = $(call compile_template,{C}  ,$(CC) ,$(CFLAGS))
COMPILE_CXX = $(call compile_template,{C++},$(CXX),$(CXXFLAGS))
COMPILE_ASM = $(call compile_template,{ASM},$(CC) -c -x assembler-with-cpp,$(ASMFLAGS))

# Search sources in ../src and ../ID_src/WinQuake
vpath %.c   $(SRC_DIR) $(IDSRC_DIR) $(STMSRC_DIR)
vpath %.cpp $(SRC_DIR) $(IDSRC_DIR) $(STMSRC_DIR)
vpath %.s $(SRC_DIR) $(IDSRC_DIR) $(STMSRC_DIR)
vpath %.S $(SRC_DIR) $(IDSRC_DIR) $(STMSRC_DIR)

# Two rules instead of four — make will find % via vpath
$(OBJ_DIR)/%.o: %.c
	$(COMPILE_C)

$(OBJ_DIR)/%.o: %.cpp
	$(COMPILE_CXX)

$(OBJ_DIR)/%.o: %.s
	$(COMPILE_ASM)


# ---------- Housekeeping ----------
clean:
	@rm -rf $(OBJ_DIR) $(TARGETS)

fresh: clean build

run: $(TARGETS)
	@cd run_env && ./quake
# 	@cd run_env && DISPLAY=:1 ./quake

rund: $(TARGETS)
	@cd run_env && DISPLAY=:1 gdbserver :1234 ./quake

# Include auto-generated deps
-include $(OBJS:.o=.d)

# Debug helpers
.PHONY: vars
vars:
	@echo "CPPFLAGS=$(CPPFLAGS)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "LDFLAGS=$(LDFLAGS)"
	@echo "LDLIBS=$(LDLIBS)"
	@echo "X11_LIBS=$(X11_LIBS)"
	@echo "SRC_LIST=$(SRC_LIST)"
	@echo "OBJS=$(OBJS)"


# include make_tools.mk
# include prog_Jet-Link.mk
include prog_ST-Link.mk
# include extra_goals.mk