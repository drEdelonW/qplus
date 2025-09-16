SRC_DIR		:= ../src
IDSRC_DIR 	:= ../ID_src/WinQuake

OBJ_DIR 	?= ./objs
OUT_DIR 	?= OUTPUT

#############################################################################
INCLUDES :=
SRC_LIST :=
include SRCS.mk
# $(info SRC_LIST = $(SRC_LIST))

MY_SRCS := $(filter $(SRC_DIR)/%,$(SRC_LIST))
MY_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MY_SRCS))

ID_SRCS := $(filter $(IDSRC_DIR)/%,$(SRC_LIST))
ID_OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(ID_SRCS:.c=.o)))

OBJS := $(MY_OBJS) $(ID_OBJS)

#############################################################################
BITS := -m32  # comment this for ARM M1

#############################################################################
CC  = gcc
CXX = g++
LD  = gcc

CFLAGS := \
	-O0 \
	-DX11 -DVID_X11 \
	-Dstricmp=strcasecmp \
	-DNO_ASM
# CFLAGS += -I/opt/homebrew/opt/libx11/include

BASE_CFLAGS = $(BITS) -Dstricmp=strcasecmp -DNO_ASM

DEBUG_CFLAGS := $(BASE_CFLAGS) -O0 -g
# XLDFLAGS = $(BITS) -L/usr/X11R6/lib -lX11 -lXext -lXxf86dga
XLDFLAGS := $(BITS) -L/usr/lib/i386-linux-gnu -lX11 -lXext
XLDFLAGS += -L/opt/homebrew/opt/libx11/lib

LDFLAGS := $(BITS) -lm

#############################################################################
# ifneq ($(NO_PARALLEL),1)
#   J ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || \
#                 nproc 2>/dev/null || \
#                 sysctl -n hw.ncpu 2>/dev/null || echo 1)
#   MAKEFLAGS += -j$(J) --output-sync=target
# endif

#############################################################################
TARGETS ?= $(OUT_DIR)/quake.x11

build:
	@mkdir -p $(OBJ_DIR) $(OUT_DIR)
	@$(MAKE) targets OBJ_DIR=./objs CFLAGS="$(DEBUG_CFLAGS)"

all: build

targets: $(TARGETS)

#############################################################################
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(XCFLAGS) -c $< -o $@ $(addprefix -I,$(INCLUDES))

$(OBJ_DIR)/%.o: $(IDSRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(XCFLAGS) -c $< -o $@ $(addprefix -I,$(INCLUDES))

#MAIN TARGET X11
$(TARGETS): $(OBJS)
	@$(LD) $(CFLAGS) -o $@ $^ $(XLDFLAGS) $(LDFLAGS)
	@cd run_env && ln -sf ../$(TARGETS) quake

clean:
	@cd $(OBJ_DIR) && 	rm -rf * && \
	 cd .. && 			rm -f $(TARGETS)

fresh: clean build

run: $(TARGETS)
	@cd run_env && DISPLAY=:1 ./quake

rund: $(TARGETS)
	cd run_env && DISPLAY=:1 gdbserver :1234 ./quake
