# ---------- Paths ----------
SRC_DIR      := ../src
IDSRC_DIR    := ../ID_src/WinQuake

OBJ_DIR      ?= ./objs
OUT_DIR      ?= OUTPUT

# ---------- Build options ----------
# DEBUG=1 for -O0 -g; DEBUG=0 for -O2 -DNDEBUG
DEBUG        ?= 1
# Set to 1 if you want 32-bit build on Linux x86_64 (requires multilibs)
FORCE_32     ?= 1
# Disable legacy ASM paths
NO_ASM       ?= 1
# X11 target on *nix
USE_X11      ?= 1

UNAME_S      := $(shell uname -s)
UNAME_M      := $(shell uname -m)
# Populated by SRCS.mk: must define SRC_LIST and optionally INCLUDES
INCLUDES     :=
SRC_LIST     :=
include SRCS.mk

# ---------- Map sources -> objects ----------
# MY_SRCS      := $(filter $(SRC_DIR)/%,$(SRC_LIST))
# MY_C_OBJS    := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter %.c,$(MY_SRCS)))
# MY_CPP_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(filter %.cpp,$(MY_SRCS)))

# ID_SRCS      := $(filter $(IDSRC_DIR)/%,$(SRC_LIST))
# ID_C_OBJS    := $(addprefix $(OBJ_DIR)/,$(notdir $(filter %.c,$(ID_SRCS:.c=.o))))
# ID_CPP_OBJS  := $(addprefix $(OBJ_DIR)/,$(notdir $(filter %.cpp,$(ID_SRCS:.cpp=.o))))

# OBJS         := $(MY_CPP_OBJS) $(MY_C_OBJS) $(ID_CPP_OBJS) $(ID_C_OBJS)

# --- order-preserving map SRC -> OBJ ---
# helper: change .c/.cpp -> .o
to_o   = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$1))

# helper: map full source path to proper object path
# - для $(SRC_DIR): сохраняем поддиректории в $(OBJ_DIR)
# - для $(IDSRC_DIR): кладём в корень $(OBJ_DIR) (как у тебя было через notdir)
map_obj = $(if $(filter $(SRC_DIR)/%,$1), \
             $(OBJ_DIR)/$(patsubst $(SRC_DIR)/%,%,$(call to_o,$1)), \
             $(if $(filter $(IDSRC_DIR)/%,$1), \
                 $(OBJ_DIR)/$(notdir $(call to_o,$1)), \
                 $(OBJ_DIR)/$(notdir $(call to_o,$1)) ) )

# ВАЖНО: порядок = порядок в SRC_LIST
OBJS := $(foreach f,$(SRC_LIST),$(call map_obj,$(f)))

# ---------- Toolchain ----------
CC           ?= gcc
CXX          ?= g++
# force LD to be CC even if env has LD=ld
# LD           := $(CC)
LD           := $(CXX)
AR           ?= ar
TIME         := /usr/bin/time -f "%E"
# \tmem=%MKB"



# ---------- Base flags ----------
# Put macros and include paths into CPPFLAGS
CPPFLAGS     := $(addprefix -I,$(INCLUDES))
CPPFLAGS     += -MMD -MP            # auto-deps
CPPFLAGS     += -Dstricmp=strcasecmp

ifeq ($(NO_ASM),1)
CPPFLAGS     += -DNO_ASM
endif

ifeq ($(USE_X11),1)
CPPFLAGS     += -DX11 -DVID_X11
endif

# GCC10+ compatibility for old C codebases
CFLAGS       ?=
CFLAGS       += -fcommon -fno-strict-aliasing -pipe
CFLAGS       += -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
                -Wno-missing-field-initializers -Wno-implicit-fallthrough \
                -Wno-trigraphs -Wno-format-truncation

CXXFLAGS     ?=
CXXFLAGS     += -fcommon -fno-strict-aliasing -pipe
CXXFLAGS     += -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
                -Wno-missing-field-initializers -Wno-implicit-fallthrough \
                -Wno-trigraphs -Wno-format-truncation

ifeq ($(DEBUG),1)
CFLAGS       += -O0 -g -fno-omit-frame-pointer
CXXFLAGS     += -O0 -g -fno-omit-frame-pointer
else
CFLAGS       += -O2 -DNDEBUG
CXXFLAGS     += -O2 -DNDEBUG
endif

# Linker flags and libraries split correctly
LDFLAGS      ?=
LDLIBS       ?= -lm

# ---------- Platform specifics ----------
ifeq ($(USE_X11),1)
  # hard fallback to avoid surprises with pkg-config
  LDLIBS += -lX11 -lXext
endif

# Optional 32-bit build on Linux x86_64
ifeq ($(UNAME_S),Linux)
  $(info UNAME_M arch: $(UNAME_M))
  ifeq ($(FORCE_32),1)
    $(info !!!USING 32bit build!!!)

    ifeq ($(UNAME_M),x86_64)
      CFLAGS   += -m32
      CXXFLAGS   += -m32
      LDFLAGS  += -m32
      # Hint pkg-config to 32-bit libs if needed (Debian/Ubuntu multilib):
      # export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig
    endif
    ifeq ($(UNAME_M),aarch64)
      CC = arm-linux-gnueabihf-gcc
      CXX = arm-linux-gnueabihf-gcc
      LD = $(CXX)
    endif
  else
  endif
else ifeq ($(UNAME_S),Darwin)
    CPPFLAGS += -I/opt/X11/include
    LDFLAGS  += -L/opt/X11/lib
else
  $(info UNAME_S $(UNAME_S))
endif

# On macOS arm64 there is no 32-bit; do nothing.

ifneq ($(NO_PARALLEL),1)
  J ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || \
                nproc 2>/dev/null || \
                sysctl -n hw.ncpu 2>/dev/null || echo 1)
#   MAKEFLAGS += -j4
# 	$(J) --output-sync=target
endif

# ---------- Targets ----------
TARGETS      ?= $(OUT_DIR)/quake.x11

.PHONY: all build targets clean fresh run rund
.NOTPARALLEL: fresh clean build

all: $(TARGETS)
build: $(TARGETS)
targets: $(TARGETS)

# Main link (use LDFLAGS/LDLIBS properly)
$(TARGETS): $(OBJS) | $(OUT_DIR)
	@$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	@cd run_env && ln -sf ../$(TARGETS) quake
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
		rc=$$?; if [ $$rc -eq 0 ]; then st=$(GRN)OK$(RST); else st=$(RED)FAIL$(RST); fi; \
		echo "$$st  $$out"; }
endef

COMPILE_C   = $(call compile_template,{C}  ,$(CC) ,$(CFLAGS))
COMPILE_CXX = $(call compile_template,{C++},$(CXX),$(CXXFLAGS))

# Search sources in ../src and ../ID_src/WinQuake
vpath %.c   $(SRC_DIR) $(IDSRC_DIR)
vpath %.cpp $(SRC_DIR) $(IDSRC_DIR)

# Two rules instead of four — make will find % via vpath
$(OBJ_DIR)/%.o: %.c
	$(COMPILE_C)

$(OBJ_DIR)/%.o: %.cpp
	$(COMPILE_CXX)


# ---------- Housekeeping ----------
clean:
	@rm -rf $(OBJ_DIR) $(TARGETS)

fresh: clean build

run: $(TARGETS)
	@cd run_env && DISPLAY=:1 ./quake

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