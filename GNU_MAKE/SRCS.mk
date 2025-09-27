
INCLUDES += $(SRC_DIR)
# INCLUDES += /opt/homebrew/opt/libx11/include
# INCLUDES += /opt/homebrew/include

$(eval VECMAT_DIR := $(SRC_DIR)/vectorMath) $(eval INCLUDES += $(VECMAT_DIR))
    SRC_LIST += $(VECMAT_DIR)/Vector3d.cpp

$(eval ENG_DIR := $(SRC_DIR)/engine) $(eval INCLUDES += $(ENG_DIR))
    $(eval GAMESPEC_DIR = $(ENG_DIR)/game_specific) $(eval INCLUDES += $(GAMESPEC_DIR))

    $(eval CORE_DIR = $(ENG_DIR)/core) $(eval INCLUDES += $(CORE_DIR))
        $(eval CLI_DIR = $(CORE_DIR)/console) $(eval INCLUDES += $(CLI_DIR))
            $(eval CVAR_DIR = $(CLI_DIR)/cvar) $(eval INCLUDES += $(CVAR_DIR))
                SRC_LIST += $(CVAR_DIR)/cvar.c
                SRC_LIST += $(CVAR_DIR)/cvar_q1.c

            $(eval CMD_DIR = $(CLI_DIR)/cmd) $(eval INCLUDES += $(CMD_DIR))
                SRC_LIST += $(CMD_DIR)/cmd.c
                SRC_LIST += $(CMD_DIR)/cmd_alias.c
                SRC_LIST += $(CMD_DIR)/host_cmd.c


            SRC_LIST += $(CLI_DIR)/console.c

        $(eval CUTILS_DIR = $(CORE_DIR)/utils) $(eval INCLUDES += $(CUTILS_DIR))
            SRC_LIST += $(CUTILS_DIR)/crc.c

        $(eval COMM_DIR = $(CORE_DIR)/common) $(eval INCLUDES += $(COMM_DIR))
            SRC_LIST += $(COMM_DIR)/common.c
            SRC_LIST += $(COMM_DIR)/msg.c
            SRC_LIST += $(COMM_DIR)/sizebuf.c
            SRC_LIST += $(COMM_DIR)/q_tools.c
            SRC_LIST += $(COMM_DIR)/endian_tools.c
            SRC_LIST += $(COMM_DIR)/link.c

        $(eval INPUT_DIR = $(CORE_DIR)/input) $(eval INCLUDES += $(INPUT_DIR))
            SRC_LIST += $(INPUT_DIR)/keys.c

        SRC_LIST += $(CORE_DIR)/host.c

    $(eval WORLD_DIR = $(ENG_DIR)/world) $(eval INCLUDES += $(WORLD_DIR))
        SRC_LIST += $(WORLD_DIR)/world.c

    $(eval NET_DIR = $(ENG_DIR)/net) $(eval INCLUDES += $(NET_DIR))
        $(eval NET_CL_DIR = $(NET_DIR)/client) $(eval INCLUDES += $(NET_CL_DIR))
            SRC_LIST += $(NET_CL_DIR)/cl_main.c
            SRC_LIST += $(NET_CL_DIR)/cl_input.c
            SRC_LIST += $(NET_CL_DIR)/cl_demo.c
            SRC_LIST += $(NET_CL_DIR)/cl_parse.c
            SRC_LIST += $(NET_CL_DIR)/cl_tent.c

        $(eval NET_SV_DIR = $(NET_DIR)/server) $(eval INCLUDES += $(NET_SV_DIR))
            SRC_LIST += $(NET_SV_DIR)/sv_main.c
            SRC_LIST += $(NET_SV_DIR)/sv_user.c
            SRC_LIST += $(NET_SV_DIR)/sv_phys.c
            SRC_LIST += $(NET_SV_DIR)/sv_move.c

        SRC_LIST += $(NET_DIR)/net_main.c
        SRC_LIST += $(NET_DIR)/net_dgrm.c
        SRC_LIST += $(NET_DIR)/net_loop.c
        SRC_LIST += $(NET_DIR)/net_vcr.c
        SRC_LIST += $(NET_DIR)/net_udp.c
        SRC_LIST += $(NET_DIR)/net_bsd.c

    $(eval PROG_DIR = $(ENG_DIR)/vm) $(eval INCLUDES += $(PROG_DIR))
        SRC_LIST += $(PROG_DIR)/pr_cmds.c
        SRC_LIST += $(PROG_DIR)/pr_edict.c
        SRC_LIST += $(PROG_DIR)/pr_exec.c

    $(eval ZONE_DIR = $(ENG_DIR)/zone) $(eval INCLUDES += $(ZONE_DIR))
        SRC_LIST += $(ZONE_DIR)/zone.c
        SRC_LIST += $(ZONE_DIR)/z_hulk.c
        SRC_LIST += $(ZONE_DIR)/z_cache.c
        SRC_LIST += $(ZONE_DIR)/z_zone.c


$(eval VID_DIR = $(SRC_DIR)/video) $(eval INCLUDES += $(VID_DIR))
    $(eval DRAW_DIR = $(VID_DIR)/draw) $(eval INCLUDES += $(DRAW_DIR))
        SRC_LIST += $(DRAW_DIR)/nonintel.c
        SRC_LIST += $(DRAW_DIR)/draw.c
        SRC_LIST += $(DRAW_DIR)/d_edge.c
        SRC_LIST += $(DRAW_DIR)/d_fill.c
        SRC_LIST += $(DRAW_DIR)/d_init.c
        SRC_LIST += $(DRAW_DIR)/d_modech.c
        SRC_LIST += $(DRAW_DIR)/d_part.c
        SRC_LIST += $(DRAW_DIR)/d_polyse.c
        SRC_LIST += $(DRAW_DIR)/d_scan.c
        SRC_LIST += $(DRAW_DIR)/d_sky.c
        SRC_LIST += $(DRAW_DIR)/d_sprite.c
        SRC_LIST += $(DRAW_DIR)/d_surf.c
        SRC_LIST += $(DRAW_DIR)/d_vars.c
        SRC_LIST += $(DRAW_DIR)/d_zpoint.c

    $(eval RENDER_DIR = $(VID_DIR)/render) $(eval INCLUDES += $(RENDER_DIR))
        SRC_LIST += $(RENDER_DIR)/r_aclip.c
        SRC_LIST += $(RENDER_DIR)/r_alias.c
        SRC_LIST += $(RENDER_DIR)/r_bsp.c
        SRC_LIST += $(RENDER_DIR)/r_light.c
        SRC_LIST += $(RENDER_DIR)/r_draw.c
        SRC_LIST += $(RENDER_DIR)/r_efrag.c
        SRC_LIST += $(RENDER_DIR)/r_edge.c
        SRC_LIST += $(RENDER_DIR)/r_misc.c
        SRC_LIST += $(RENDER_DIR)/r_main.c
        SRC_LIST += $(RENDER_DIR)/r_sky.c
        SRC_LIST += $(RENDER_DIR)/r_sprite.c
        SRC_LIST += $(RENDER_DIR)/r_surf.c
        SRC_LIST += $(RENDER_DIR)/r_part.c
        SRC_LIST += $(RENDER_DIR)/r_vars.c

$(eval SND_DIR = $(SRC_DIR)/sound) $(eval INCLUDES += $(SND_DIR))
    SRC_LIST += $(SND_DIR)/snd_dma.c
    SRC_LIST += $(SND_DIR)/snd_mem.c
    SRC_LIST += $(SND_DIR)/snd_mix.c

$(eval MATH_DIR = $(SRC_DIR)/math) $(eval INCLUDES += $(MATH_DIR))
  SRC_LIST += $(MATH_DIR)/mathlib.c

$(eval AST_DIR = $(SRC_DIR)/assets) $(eval INCLUDES += $(AST_DIR))
    $(eval MDL_DIR = $(AST_DIR)/model) $(eval INCLUDES += $(MDL_DIR))
        SRC_LIST += $(MDL_DIR)/model.c

    $(eval WAD_DIR = $(AST_DIR)/wad) $(eval INCLUDES += $(WAD_DIR))
        SRC_LIST += $(WAD_DIR)/wad.c

$(eval UI_DIR = $(SRC_DIR)/ui) $(eval INCLUDES += $(UI_DIR))
    $(eval MENU_DIR = $(UI_DIR)/menu) $(eval INCLUDES += $(MENU_DIR))
        SRC_LIST += $(MENU_DIR)/menu.c
        SRC_LIST += $(MENU_DIR)/menu_common.c

    SRC_LIST += $(UI_DIR)/screen.c
    SRC_LIST += $(UI_DIR)/sbar.c
    SRC_LIST += $(UI_DIR)/view.c
    SRC_LIST += $(UI_DIR)/chase.c

$(eval PLATFORM_DIR = $(SRC_DIR)/platform) $(eval INCLUDES += $(PLATFORM_DIR))
ifeq ($(UNAME_S),Linux)
    $(eval POSIX_DIR = $(PLATFORM_DIR)/posix) $(eval INCLUDES += $(POSIX_DIR))
        SRC_LIST += $(POSIX_DIR)/sys_linux.c
        SRC_LIST += $(POSIX_DIR)/vid_x.c
        SRC_LIST += $(POSIX_DIR)/cd_linux.c
        SRC_LIST += $(POSIX_DIR)/snd_linux.c

else ifeq ($(UNAME_S),Darwin)
    $(eval POSIX_DIR = $(PLATFORM_DIR)/posix) $(eval INCLUDES += $(POSIX_DIR))
        SRC_LIST += $(POSIX_DIR)/sys_linux.c
        SRC_LIST += $(POSIX_DIR)/vid_x.c
    # macOS build: disable X11/SHM, use NULL stubs
    SRC_LIST += $(PLATFORM_DIR)/null/cd_null.c
    SRC_LIST += $(PLATFORM_DIR)/null/snd_null.c
    # NOTE: skip xshm_stubs.c to avoid missing X11 headers
    CFLAGS  += -DNO_X11_SHM

else
    SRC_LIST += $(PLATFORM_DIR)/null/cd_null.c
    SRC_LIST += $(PLATFORM_DIR)/null/snd_null.c
    SRC_LIST += $(PLATFORM_DIR)/null/xshm_stubs.c
endif

$(eval OTHER_DIR = $(SRC_DIR)/etc) $(eval INCLUDES += $(OTHER_DIR))

