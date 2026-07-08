# Shared GCC-dialect flags for all desktop toolchains (POSIX, WIN).
# Included at the end of GCC_tools_POSIX.mk / GCC_tools_WIN.mk.
# Anything platform-specific (portability shims, per-OS libs) belongs in
# the including file instead, not here.

CXXFLAGS     += -MMD -MP            # auto-deps

ifeq ($(NO_ASM),1)
  CXXFLAGS     += -DNO_ASM
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
