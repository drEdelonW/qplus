TRG_PREFIX 		?= arm-none-eabi
# GCC_VER			?= 11.3.rel1
GCC_VER			?= 12.3.rel1
# GCC_VER			?= 13.2.Rel1
# GCC_VER			?= 13.3.Rel1

TRG_PREFIX  ?= arm-none-eabi
GCC_VER     ?= 12.3.rel1

ARCH    ?= x64
# ARCH    ?= x86

#==============OS DEPENDENT VARS================
UNAME_S   ?= $(shell uname -s)

ifeq ($(OS),Windows_NT)
$(info NT)
    NUM_CORES ?= $(shell wmic cpu get NumberOfLogicalProcessors | grep -vE "^[^0-9]+$ ")
    ARCH      := mingw-w64-i686
    PKG_SUFIX := .zip

    # GCC_BASE_PATH    := /ST/STM32CubeIDE_1.14.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.11.3.rel1.win32_1.1.100.202309141235/tools/
    # GCC_BASE_PATH    := /ST/STM32CubeIDE_1.15.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.12.3.rel1.win32_1.0.100.202403111256/tools/
    GCC_BASE_PATH    := ~/arm-gnu-toolchain-$(GCC_VER)-mingw-w64-i686-$(TRG_PREFIX)/
    SUB_PATH         := $(shell echo $(GCC_BASE_PATH) | sed 's|^/c/|C:/|')bin/
    GCC_PATH         ?= $(GCC_BASE_PATH)/bin/

    OBJCOPY  = $(SUB_PATH)$(TRG_PREFIX)-objcopy
    SIZE     = $(SUB_PATH)$(TRG_PREFIX)-size

    ifeq ($(UNAME_S), MINGW64_NT-10.0-19045)
    $(info Windows 10 Bash build)

    else ifeq ($(UNAME_S), MINGW64_NT-10.0-22631)
    $(info Windows 11 Bash build)
        PYTHON            ?= python
    endif
else ifeq ($(UNAME_S), Darwin)
$(info MacOS zsh build)
    NUM_CORES ?= $(shell sysctl -n hw.ncpu)
    UNAME_M   ?= $(shell uname -m)
    ifeq ($(UNAME_M),arm64)
        ARCH  := darwin-arm64
    else ifeq ($(UNAME_M),x86_64)
        ARCH  := darwin-x86_64
    endif
    PKG_SUFIX := .pkg


    GCC_BASE_PATH    := /Applications/STM32CubeIDE.app/Contents/Eclipse/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.macos64_1.0.0.202411102158/tools/
#     GCC_BASE_PATH    := /Applications/ArmGNUToolchain/$(GCC_VER)/$(TRG_PREFIX)/
    GCC_PATH         ?= $(GCC_BASE_PATH)bin/
else ifeq ($(UNAME_S), Linux)
$(info Linux build)
    NUM_CORES        ?= $(shell nproc)
    ARCH             := $(shell uname -m)
    PKG_SUFIX        := .tar.xz

    GCC_BASE_PATH    ?= ~/arm-gnu-toolchain-$(GCC_VER)-$(ARCH)-$(TRG_PREFIX)/
    GCC_PATH         ?= $(GCC_BASE_PATH)bin/
else
    $(info UNAME_S is $(UNAME_S))
    $(error Unknown build)
endif

check_dir_exists = \
    $(if $(wildcard $(GCC_BASE_PATH)), \
        $(info Directory $(GCC_BASE_PATH) exists.), \
        $(info Directory $(GCC_BASE_PATH) does not exist.) \
        $(error Download [https://developer.arm.com/-/media/Files/downloads/gnu/$(GCC_VER)/binrel/arm-gnu-toolchain-$(GCC_VER)-$(ARCH)-arm-none-eabi$(PKG_SUFIX)]))
# Call the function at the beginning of the Makefile
$(call check_dir_exists)


#==============COMPILATION VARS================
CC      =  $(GCC_PATH)$(TRG_PREFIX)-gcc
CXX     =  $(GCC_PATH)$(TRG_PREFIX)-g++
LD      =  $(GCC_PATH)$(TRG_PREFIX)-g++
OBJCOPY ?= $(GCC_PATH)$(TRG_PREFIX)-objcopy
SIZE    ?= $(GCC_PATH)$(TRG_PREFIX)-size
PYTHON  ?= python3

CFLAGS   ?=
CXXFLAGS ?=
LDFLAGS  ?=

# CFLAGS   += 
# CXXFLAGS += 
# LDFLAGS  += 

LDFLAGS += -specs=nosys.specs -Wl,--gc-sections
LDLIBS  += -lc -lm -lnosys