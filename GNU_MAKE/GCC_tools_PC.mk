
CC           ?= gcc
CXX          ?= g++
# force LD to be CC even if env has LD=ld
LD           := $(CC)
# LD           := $(CXX)
# AR           ?= ar

#==============OS DEPENDENT VARS================
# ---------- Platform specifics ----------
ifeq ($(OS),Windows_NT)
    $(info Windows_NT build)
  # ==== 32-bit toggle on Windows (MSYS2/MinGW) ====
    ifeq ($(FORCE_32),1)
        $(info !!! USING 32-bit MinGW toolchain !!!)
        GCC_PATH := /c/msys64/mingw32/bin
        MINGW_PREFIX := $(GCC_PATH)/i686-w64-mingw32

        CFLAGS   += -m32
        CXXFLAGS += -m32
        LDFLAGS  += -m32
    else
        GCC_PATH := /c/msys64/mingw64/bin
        MINGW_PREFIX := $(GCC_PATH)/x86_64-w64-mingw32
    endif

    CC  := $(MINGW_PREFIX)-gcc
    CXX := $(MINGW_PREFIX)-g++
    LD  := $(CC)

else ifeq ($(UNAME_S),Linux)
  $(info Linux build)
#   TIME := /usr/bin/time -f "%E"

  $(info UNAME_M arch: $(UNAME_M))
  ifeq ($(FORCE_32),1)
    $(info !!!USING 32bit build!!!)

    ifeq ($(UNAME_M),x86_64)
      CFLAGS   += -m32
      CXXFLAGS += -m32
      LDFLAGS  += -m32
      # Hint pkg-config to 32-bit libs if needed (Debian/Ubuntu multilib):
      # export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig
    endif
    ifeq ($(UNAME_M),aarch64)
      CC = arm-linux-gnueabihf-gcc
      CXX = arm-linux-gnueabihf-g++
      LD = $(CXX)
    endif
  else
  endif
else ifeq ($(UNAME_S),Darwin)
  $(info Darwin build)
    ECHO = echo
 #     CFLAGS   += -m32
 #     CXXFLAGS += -m32
 #     LDFLAGS  += -m32

    CC = gcc
    CXX = gcc
    LD = $(CC)

    CFLAGS  += -D__APPLE__ -DBSD
else
  TIME :=
  ifeq ($(FORCE_32),1)
    CFLAGS   += -m32
    CXXFLAGS += -m32
    LDFLAGS  += -m32
  else
  endif
  $(info UNAME_S $(UNAME_S))
endif

# ---------- Base flags ----------
# Put macros and include paths into CPPFLAGS
CPPFLAGS     += -MMD -MP            # auto-deps
CPPFLAGS     += -Dstricmp=strcasecmp

ifeq ($(NO_ASM),1)
  CPPFLAGS     += -DNO_ASM
endif

# GCC10+ compatibility for old C codebases
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
  CFLAGS     += -O0 -g -fno-omit-frame-pointer
  CXXFLAGS   += -O0 -g -fno-omit-frame-pointer
else
  CFLAGS     += -O2 -DNDEBUG
  CXXFLAGS   += -O2 -DNDEBUG
endif

# Linker flags and libraries split correctly
LDLIBS       += -lm
