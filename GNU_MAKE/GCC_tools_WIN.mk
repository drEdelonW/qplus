# Toolchain for producing a Windows binary, regardless of build host.
# Used by DST_PLATFORM=WIN targets: quakeWin.
# Native Windows host -> MSYS2/MinGW. Linux/macOS host -> cross-compile via
# the mingw-w64 packages (apt install mingw-w64 / g++-mingw-w64).
# For native desktop builds that target whatever host they run on, see
# GCC_tools_POSIX.mk (DST_PLATFORM=POSIX).

CC           ?= gcc
CXX          ?= g++
# force LD to be CC even if env has LD=ld
LD           := $(CC)
# AR           ?= ar

# ---- Architecture: FORCE_32=1 forces the i686 (32-bit) mingw-w64 triplet;
#      64-bit (x86_64) is the default now that the engine's pointer-arithmetic
#      bugs are fixed. Honored on any host that runs this file (STM32 never
#      includes GCC_tools_WIN.mk).
ifeq ($(FORCE_32),1)
    $(info !!! USING 32-bit MinGW toolchain !!!)
    PC_ARCH  := i686

    CFLAGS   += -m32
    CXXFLAGS += -m32
    LDFLAGS  += -m32
else
    PC_ARCH  := x86_64
endif

# ---- Toolchain location: native MSYS2 install on Windows, mingw-w64
#      cross-compiler package on Linux/macOS.
ifeq ($(OS),Windows_NT)
    $(info Windows_NT build (native MSYS2/MinGW), PC_ARCH=$(PC_ARCH))
    ifeq ($(PC_ARCH),i686)
        GCC_PATH := /mingw32/bin
    else
        GCC_PATH := /mingw64/bin
    endif
#     MINGW_PREFIX := $(GCC_PATH)/$(PC_ARCH)-w64-mingw32-
    MINGW_PREFIX := $(GCC_PATH)/

    CC  := $(MINGW_PREFIX)gcc
    CXX := $(MINGW_PREFIX)g++
else
    $(info Cross-building Windows target from $(UNAME_S) host via mingw-w64, PC_ARCH=$(PC_ARCH))
    CROSS_PREFIX := $(PC_ARCH)-w64-mingw32-

    CC  := $(CROSS_PREFIX)gcc
    CXX := $(CROSS_PREFIX)g++
    AR  := $(CROSS_PREFIX)ar
endif

LD  := $(CC)

include GCC_tools_common.mk
