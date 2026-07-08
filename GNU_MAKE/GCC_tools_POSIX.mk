# Native desktop toolchain for POSIX hosts (Linux/macOS) and unknown-host
# fallback. Used by DST_PLATFORM=POSIX targets: quakeX11, quakeGLX11,
# testX11GL, testSDL2 — always builds *for* the host it runs on.
# For cross-compiling a Windows binary regardless of host, see
# GCC_tools_WIN.mk (DST_PLATFORM=WIN).

CC           ?= gcc
CXX          ?= g++
# force LD to be CC even if env has LD=ld
LD           := $(CC)
# LD           := $(CXX)
# AR           ?= ar

#==============OS DEPENDENT VARS================
# ---------- Platform specifics ----------
ifeq ($(UNAME_S),Linux)
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

# WinQuake source calls stricmp(), which has no POSIX equivalent by that
# name (glibc/libSystem only provide strcasecmp). Native Windows builds
# already have stricmp in their CRT, so this shim belongs here, not in
# GCC_tools_common.mk.
CXXFLAGS     += -Dstricmp=strcasecmp

include GCC_tools_common.mk
