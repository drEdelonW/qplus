DST_PLATFORM := PC

INCLUDES += $(SRC_DIR)
include features/fh_qEngine.mk

$(eval PLATFORM_DIR = $(SRC_DIR)/platform) $(eval INCLUDES += $(PLATFORM_DIR)) $(eval INCLUDES += $(PLATFORM_DIR)/API)

#----------LINUX------------
ifeq ($(UNAME_S),Linux)
    $(info Linux X11)
    $(eval POSIX_DIR = $(PLATFORM_DIR)/posix) $(eval INCLUDES += $(POSIX_DIR))
        SRC_LIST += $(POSIX_DIR)/sys_linux.c
        SRC_LIST += $(POSIX_DIR)/sys_linux_file.c
        SRC_LIST += $(POSIX_DIR)/in_x.c
        SRC_LIST += $(POSIX_DIR)/cd_linux.c
        # SRC_LIST += $(POSIX_DIR)/snd_linux.c  # TODO:
        SRC_LIST += $(POSIX_DIR)/net_udp.c
        SRC_LIST += $(POSIX_DIR)/net_bsd.c


        # X11 target on *nix
        CXXFLAGS   += -DX11 -DVID_X11
       # CFLAGS   += -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion \
-Wint-to-pointer-cast -Wpointer-to-int-cast -Wformat=2 \
-Werror=conversion -Werror=sign-conversion -Werror=int-to-pointer-cast -Werror=pointer-to-int-cast

        LDLIBS     += -lX11 -lXext

#----------MacOS------------
else ifeq ($(UNAME_S),Darwin)
    $(info Darwin X11)
#     INCLUDES += /opt/homebrew/opt/libx11/include
    INCLUDES += /opt/homebrew/include
    $(eval POSIX_DIR = $(PLATFORM_DIR)/posix) $(eval INCLUDES += $(POSIX_DIR))
        SRC_LIST += $(POSIX_DIR)/sys_linux.c
        SRC_LIST += $(POSIX_DIR)/in_x.c
        SRC_LIST += $(POSIX_DIR)/net_udp.c
        SRC_LIST += $(POSIX_DIR)/net_bsd.c


    # macOS build: disable X11/SHM, use NULL stubs
    # NOTE: skip xshm_stubs.c to avoid missing X11 headers
    CFLAGS  += -DNO_X11_SHM
    # X11 target on *nix
    CXXFLAGS += -DX11 -DVID_X11
    CXXFLAGS += -I/opt/X11/include

    LDFLAGS  += -L/opt/X11/lib
    LDLIBS     += -lX11 -lXext
    FORCE_32     := 0
else
    $(eval PL_NULL_DIR = $(PLATFORM_DIR)/null) $(eval INCLUDES += $(PL_NULL_DIR))
        SRC_LIST += $(PL_NULL_DIR)/xshm_stubs.c
endif

