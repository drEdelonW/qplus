# Set to 1 if you want 32-bit build on Linux x86_64 (requires multilibs)
FORCE_32     ?= 1

$(eval ENG_DIR := $(SRC_DIR)/engine) $(eval INCLUDES += $(ENG_DIR))
        $(eval STRUCT_DIR := $(ENG_DIR)/structs) $(eval INCLUDES += $(STRUCT_DIR))
                SRC_LIST += $(STRUCT_DIR)/qPic.c
                SRC_LIST += $(STRUCT_DIR)/pcx.c
                SRC_LIST += $(STRUCT_DIR)/Pak.c

        $(eval GAMESPEC_DIR = $(ENG_DIR)/game_specific) $(eval INCLUDES += $(GAMESPEC_DIR))

        $(eval CORE_DIR = $(ENG_DIR)/core) $(eval INCLUDES += $(CORE_DIR))
                $(eval CLI_DIR = $(CORE_DIR)/console) $(eval INCLUDES += $(CLI_DIR))
                        $(eval CVAR_DIR = $(CLI_DIR)/cvar) $(eval INCLUDES += $(CVAR_DIR))
                                SRC_LIST += $(CVAR_DIR)/cvar.c
                                SRC_LIST += $(CVAR_DIR)/cvar_q1.c

                        $(eval CMD_DIR = $(CLI_DIR)/cmd) $(eval INCLUDES += $(CMD_DIR))
                                $(eval CBUF_DIR = $(CMD_DIR)/Cbuf) $(eval INCLUDES += $(CBUF_DIR))
#                                         SRC_LIST += $(CBUF_DIR)/cbuf.c
                                        SRC_LIST += $(CBUF_DIR)/cbuf_cWrap.cpp
                                        SRC_LIST += $(CBUF_DIR)/cbuf_obj.cpp

                                SRC_LIST += $(CMD_DIR)/cmd.c
                                SRC_LIST += $(CMD_DIR)/cmd_alias.c

                        SRC_LIST += $(CLI_DIR)/console.c

                $(eval CUTILS_DIR = $(CORE_DIR)/utils) $(eval INCLUDES += $(CUTILS_DIR))
                        SRC_LIST += $(CUTILS_DIR)/crc.c
                        SRC_LIST += $(CUTILS_DIR)/endian_tools.c

                $(eval COMM_DIR = $(CORE_DIR)/common) $(eval INCLUDES += $(COMM_DIR))

                        SRC_LIST += $(COMM_DIR)/common.c
                        SRC_LIST += $(COMM_DIR)/sizebuf.c
                        SRC_LIST += $(COMM_DIR)/q_tools.c
                        SRC_LIST += $(COMM_DIR)/link.c

                $(eval INPUT_DIR = $(CORE_DIR)/input) $(eval INCLUDES += $(INPUT_DIR))
                        SRC_LIST += $(INPUT_DIR)/keys.c

                $(eval HOST_DIR = $(CORE_DIR)/Host) $(eval INCLUDES += $(HOST_DIR))
#                         SRC_LIST += $(HOST_DIR)/host.c
                        SRC_LIST += $(HOST_DIR)/host_cmd.c
                        SRC_LIST += $(HOST_DIR)/host_obj.cpp
                        SRC_LIST += $(HOST_DIR)/host_cWrap.cpp
                        SRC_LIST += $(HOST_DIR)/host_cUnWrapped.cpp

        $(eval WORLD_DIR = $(ENG_DIR)/world) $(eval INCLUDES += $(WORLD_DIR))
                SRC_LIST += $(WORLD_DIR)/world.c

        $(eval NET_DIR = $(ENG_DIR)/net) $(eval INCLUDES += $(NET_DIR))
                $(eval MSG_DIR = $(NET_DIR)/Msg) $(eval INCLUDES += $(MSG_DIR))
#                         SRC_LIST += $(MSG_DIR)/msg.c
                        SRC_LIST += $(MSG_DIR)/msg_cWrap.cpp
                        SRC_LIST += $(MSG_DIR)/msg_obj.cpp

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

        $(eval AST_DIR = $(ENG_DIR)/assets) $(eval INCLUDES += $(AST_DIR))
                $(eval MDL_DIR = $(AST_DIR)/model) $(eval INCLUDES += $(MDL_DIR))
                        SRC_LIST += $(MDL_DIR)/model.c

                $(eval WAD_DIR = $(AST_DIR)/wad) $(eval INCLUDES += $(WAD_DIR))
                        SRC_LIST += $(WAD_DIR)/wad.c

        $(eval PROG_DIR = $(ENG_DIR)/vm) $(eval INCLUDES += $(PROG_DIR))
                SRC_LIST += $(PROG_DIR)/pr_cmds.c
                SRC_LIST += $(PROG_DIR)/pr_edict.c
                SRC_LIST += $(PROG_DIR)/pr_exec.c

        $(eval UI_DIR = $(ENG_DIR)/ui) $(eval INCLUDES += $(UI_DIR))
                $(eval MENU_DIR = $(UI_DIR)/menu) $(eval INCLUDES += $(MENU_DIR))
                        SRC_LIST += $(MENU_DIR)/menu.c
                        SRC_LIST += $(MENU_DIR)/menu_common.c
                        SRC_LIST += $(MENU_DIR)/menu_main.c
                        SRC_LIST += $(MENU_DIR)/menu_singleplayer.c
                        SRC_LIST += $(MENU_DIR)/menu_load_save.c
                        SRC_LIST += $(MENU_DIR)/menu_multiplayer.c
                        SRC_LIST += $(MENU_DIR)/menu_setup.c
                        SRC_LIST += $(MENU_DIR)/menu_net.c
                        SRC_LIST += $(MENU_DIR)/menu_search.c
                        SRC_LIST += $(MENU_DIR)/menu_server_list.c
                        SRC_LIST += $(MENU_DIR)/menu_config_serial.c
                        SRC_LIST += $(MENU_DIR)/menu_config_modem.c
                        SRC_LIST += $(MENU_DIR)/menu_config_lan.c
                        SRC_LIST += $(MENU_DIR)/menu_options.c
                        SRC_LIST += $(MENU_DIR)/menu_game_options.c
                        SRC_LIST += $(MENU_DIR)/menu_defkey.c
                        SRC_LIST += $(MENU_DIR)/menu_help.c
                        SRC_LIST += $(MENU_DIR)/menu_quit.c

                SRC_LIST += $(UI_DIR)/screen.c
                SRC_LIST += $(UI_DIR)/sbar.c
                SRC_LIST += $(UI_DIR)/view.c
                SRC_LIST += $(UI_DIR)/chase.c

        $(eval ZONE_DIR = $(ENG_DIR)/zone) $(eval INCLUDES += $(ZONE_DIR))
                SRC_LIST += $(ZONE_DIR)/zone.c
                SRC_LIST += $(ZONE_DIR)/z_hulk.c
                SRC_LIST += $(ZONE_DIR)/z_cache.c
                SRC_LIST += $(ZONE_DIR)/z_zone.c

        $(eval VID_DIR = $(ENG_DIR)/video) $(eval INCLUDES += $(VID_DIR))
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

        $(eval SND_DIR = $(ENG_DIR)/sound) $(eval INCLUDES += $(SND_DIR))
                SRC_LIST += $(SND_DIR)/snd_dma.c
                SRC_LIST += $(SND_DIR)/snd_mem.c
                SRC_LIST += $(SND_DIR)/snd_mix.c

        $(eval MATH_DIR = $(ENG_DIR)/math) $(eval INCLUDES += $(MATH_DIR))
                SRC_LIST += $(MATH_DIR)/mathlib.c
                SRC_LIST += $(MATH_DIR)/Vector3d.cpp
                SRC_LIST += $(MATH_DIR)/vector.cpp
                SRC_LIST += $(MATH_DIR)/angle.c
