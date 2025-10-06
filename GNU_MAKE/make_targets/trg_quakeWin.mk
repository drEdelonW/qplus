DST_PLATFORM := PC

INCLUDES += $(SRC_DIR)/platform/Windows
# INCLUDES += $(IDSRC_DIR)

include features/fh_qEngine.mk

$(eval PLATFORM_DIR = $(SRC_DIR)/platform) $(eval INCLUDES += $(PLATFORM_DIR)/API)
    $(eval WIN_DIR = $(PLATFORM_DIR)/Windows) $(eval INCLUDES += $(WIN_DIR))
        SRC_LIST += $(WIN_DIR)/conproc.c
        SRC_LIST += $(WIN_DIR)/net_wipx.c # ?
        SRC_LIST += $(WIN_DIR)/net_win.c
        SRC_LIST += $(WIN_DIR)/net_wins.c
        SRC_LIST += $(WIN_DIR)/in_win.c
        SRC_LIST += $(WIN_DIR)/snd_win.c
        SRC_LIST += $(WIN_DIR)/sys_win.c
#         SRC_LIST += $(WIN_DIR)/sys_wind.c
        SRC_LIST += $(WIN_DIR)/vid_win.c
        SRC_LIST += $(WIN_DIR)/cd_win.c
#         SRC_LIST += $(WIN_DIR)/mgl_stubs.c
        SRC_LIST += $(WIN_DIR)/fpu_stubs.c

    LDLIBS += $(WIN_DIR)/MGLLT.LIB -luser32 -lgdi32 -lwinmm -lws2_32 -lwsock32 -ldxguid
    DEFINES += _WIN32
