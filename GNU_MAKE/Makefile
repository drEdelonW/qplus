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

# Populated by SRCS.mk: must define SRC_LIST and optionally INCLUDES
INCLUDES     :=
SRC_LIST     :=
include SRCS.mk

# ---------- Map sources -> objects ----------
MY_SRCS      := $(filter $(SRC_DIR)/%,$(SRC_LIST))
MY_C_OBJS    := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter %.c,$(MY_SRCS)))
MY_CPP_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(filter %.cpp,$(MY_SRCS)))

ID_SRCS      := $(filter $(IDSRC_DIR)/%,$(SRC_LIST))
ID_C_OBJS    := $(addprefix $(OBJ_DIR)/,$(notdir $(filter %.c,$(ID_SRCS:.c=.o))))
ID_CPP_OBJS  := $(addprefix $(OBJ_DIR)/,$(notdir $(filter %.cpp,$(ID_SRCS:.cpp=.o))))

OBJS         := $(MY_C_OBJS) $(MY_CPP_OBJS) $(ID_C_OBJS) $(ID_CPP_OBJS)

# ---------- Toolchain ----------
CC           ?= gcc
CXX          ?= g++
# force LD to be CC even if env has LD=ld
# LD           := $(CC)
LD           := $(CXX)
AR           ?= ar

UNAME_S      := $(shell uname -s)
UNAME_M      := $(shell uname -m)

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
  ifeq ($(UNAME_M),x86_64)
    ifeq ($(FORCE_32),1)
      CFLAGS   += -m32
      LDFLAGS  += -m32
      # Hint pkg-config to 32-bit libs if needed (Debian/Ubuntu multilib):
      # export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig
    endif
  endif
endif

# On macOS arm64 there is no 32-bit; do nothing.

# ---------- Targets ----------
TARGETS      ?= $(OUT_DIR)/quake.x11

.PHONY: all build targets clean fresh run rund

all: $(TARGETS)
build: $(TARGETS)
targets: $(TARGETS)

# Main link (use LDFLAGS/LDLIBS properly)
$(TARGETS): $(OBJS) | $(OUT_DIR)
	@$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	@cd run_env && ln -sf ../$(TARGETS) quake

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

# ---------- Compile rules ----------
# Note: use CPPFLAGS for -D/-I, CFLAGS for codegen; generate .d files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(IDSRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(IDSRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

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