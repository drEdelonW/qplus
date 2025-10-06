DST_PLATFORM := PC

INCLUDES += $(SRC_DIR)
# INCLUDES += /opt/homebrew/opt/libx11/include
# INCLUDES += /opt/homebrew/include

include features/fh_qEngine.mk

$(eval PLATFORM_DIR = $(SRC_DIR)/platform) $(eval INCLUDES += $(PLATFORM_DIR)/API)

ifeq ($(UNAME_S),Linux)
        $(eval POSIX_DIR = $(PLATFORM_DIR)/posix) $(eval INCLUDES += $(POSIX_DIR))
                SRC_LIST += $(POSIX_DIR)/sys_linux.c
                SRC_LIST += $(POSIX_DIR)/vid_x.c
                SRC_LIST += $(POSIX_DIR)/cd_linux.c
                SRC_LIST += $(POSIX_DIR)/snd_linux.c
                SRC_LIST += $(POSIX_DIR)/net_udp.c
                SRC_LIST += $(POSIX_DIR)/net_bsd.c

        LDLIBS += -lX11 -lXext

else ifeq ($(UNAME_S),Darwin)
        $(eval POSIX_DIR = $(PLATFORM_DIR)/posix) $(eval INCLUDES += $(POSIX_DIR))
                SRC_LIST += $(POSIX_DIR)/sys_linux.c
                SRC_LIST += $(POSIX_DIR)/vid_x.c
                SRC_LIST += $(PLATFORM_DIR)/null/cd_null.c
                SRC_LIST += $(PLATFORM_DIR)/null/snd_null.c
                SRC_LIST += $(POSIX_DIR)/net_udp.c
                SRC_LIST += $(POSIX_DIR)/net_bsd.c

    # macOS build: disable X11/SHM, use NULL stubs
    # NOTE: skip xshm_stubs.c to avoid missing X11 headers
        CFLAGS  += -DNO_X11_SHM

else
        SRC_LIST += $(PLATFORM_DIR)/null/cd_null.c
        SRC_LIST += $(PLATFORM_DIR)/null/snd_null.c
        SRC_LIST += $(PLATFORM_DIR)/null/xshm_stubs.c
endif

$(eval OTHER_DIR = $(SRC_DIR)/etc) $(eval INCLUDES += $(OTHER_DIR))

