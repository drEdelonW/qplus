
BITS := -m32

BUILD_DEBUG_DIR 	:= OUTPUT
# BUILD_RELEASE_DIR 	:= ../release_
CFLAGS := -O0 -DX11 -DVID_X11 -Dstricmp=strcasecmp -DNO_ASM

CC = gcc
BASE_CFLAGS = $(BITS) -Dstricmp=strcasecmp -DNO_ASM
# RELEASE_CFLAGS = $(BASE_CFLAGS) -g -mpentiumpro -O6 -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations
DEBUG_CFLAGS := $(BASE_CFLAGS) -O0 -g
# XLDFLAGS = $(BITS) -L/usr/X11R6/lib -lX11 -lXext -lXxf86dga
XLDFLAGS := $(BITS) -L/usr/lib/i386-linux-gnu -lX11 -lXext -lXxf86dga
XCFLAGS = -DX11

DO_X11_CC 		= $(CC) $(CFLAGS) $(XCFLAGS) -o $@ -c $<
DO_X11_DEBUG_CC = $(CC) $(DEBUG_CFLAGS) $(XCFLAGS) -o $@ -c $<

LDFLAGS := $(BITS) -lm

#############################################################################
# SETUP AND BUILD
#############################################################################

TARGETS ?= $(BUILD_DEBUG_DIR)/quake.x11

ifneq ($(NO_PARALLEL),1)
  J ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || \
                nproc 2>/dev/null || \
                sysctl -n hw.ncpu 2>/dev/null || echo 1)
  MAKEFLAGS += -j$(J) --output-sync=target
endif

build_debug:
	@mkdir -p $(BUILD_DEBUG_DIR) \
		$(BUILD_DEBUG_DIR) \
		./objs
	@$(MAKE) targets OBJ_DIR=./objs CFLAGS="$(DEBUG_CFLAGS)"

# build_release:
# 	@mkdir -p $(BUILD_RELEASE_DIR) \
# 		$(BUILD_RELEASE_DIR)/bin \
# 		$(BUILD_RELEASE_DIR)/x11
# 	@$(MAKE) targets OBJ_DIR=$(BUILD_RELEASE_DIR)/x11 CFLAGS="$(RELEASE_CFLAGS)"

all: build_debug
#  build_release

targets: $(TARGETS)

#############################################################################
# X11 Quake
#############################################################################
OBJ_DIR 	:= ./objs
SRC_DIR		:= ../src
IDSRC_DIR 	:= ../ID_src/WinQuake

INCLUDES :=
SRC_LIST :=
include SRCS.mk
# $(info SRC_LIST = $(SRC_LIST))


MY_SRCS  := $(filter $(SRC_DIR)/%,$(SRC_LIST))
ID_SRCS  := $(filter $(IDSRC_DIR)/%,$(SRC_LIST))

# OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(filter %.c,$(SRC_LIST))) \
# 		$(patsubst %.cpp,$(OBJ_DIR)/%.o,$(filter %.cpp,$(SRC_LIST))) #\
#         $(OBJ_DIR)/common.o

MY_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MY_SRCS))
ID_OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(ID_SRCS:.c=.o)))

# OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(SRC_LIST:.c=.o)))
OBJS := $(MY_OBJS) $(ID_OBJS)

DO_X11_CC ?= $(CC) $(CFLAGS) -DX11 -o $@ -c $<

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(XCFLAGS) -c $< -o $@ $(addprefix -I,$(INCLUDES))

$(OBJ_DIR)/%.o : $(IDSRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(XCFLAGS) -c $< -o $@ $(addprefix -I,$(INCLUDES))

# EXEPTION FOR common.o
$(OBJ_DIR)/common.o : DO_X11_CC = $(DO_X11_DEBUG_CC)


#MAIN TARGET X11
$(BUILD_DEBUG_DIR)/quake.x11: $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $^ $(XLDFLAGS) $(LDFLAGS)
	@cd run_env && ln -sf ../OUTPUT/quake.x11 quake

clean:
	@cd $(OBJ_DIR) && rm -rf * && cd .. && rm -f  $(BUILD_DEBUG_DIR)/quake.x11

fresh: clean build_debug

run: $(BUILD_DEBUG_DIR)/quake.x11
	@cd run_env && DISPLAY=:1 ./quake

rund: $(BUILD_DEBUG_DIR)/quake.x11
	cd run_env && DISPLAY=:1 gdbserver :1234 ./quake
