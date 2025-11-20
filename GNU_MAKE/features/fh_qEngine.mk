# Set to 1 if you want 32-bit build on Linux x86_64 (requires multilibs)
FORCE_32     ?= 1

$(eval ENG_DIR := $(SRC_DIR)/engine) $(eval INCLUDES += $(ENG_DIR))

        $(eval HOST_DIR = $(ENG_DIR)/Host) $(eval INCLUDES += $(HOST_DIR))
                $(eval API_DIR := $(HOST_DIR)/API) $(eval INCLUDES += $(API_DIR))

                $(eval PLAPI_DIR := $(HOST_DIR)/platformAPI) $(eval INCLUDES += $(PLAPI_DIR))
                        #SRC_LIST += $(PLAPI_DIR)/sys_null.c
                        SRC_LIST += $(PLAPI_DIR)/vid_null.c
                        SRC_LIST += $(PLAPI_DIR)/in_null.c
                        SRC_LIST += $(PLAPI_DIR)/snd_null.c
                        SRC_LIST += $(PLAPI_DIR)/cd_null.c

                #SRC_LIST += $(HOST_DIR)/host.c
                SRC_LIST += $(HOST_DIR)/host_cmd.c
                SRC_LIST += $(HOST_DIR)/host_obj.cpp
                SRC_LIST += $(HOST_DIR)/host_cWrap.cpp
                SRC_LIST += $(HOST_DIR)/host_cUnWrapped.cpp

                $(eval CLI_DIR = $(HOST_DIR)/console) $(eval INCLUDES += $(CLI_DIR))
                        $(eval CVAR_DIR = $(CLI_DIR)/cvar) $(eval INCLUDES += $(CVAR_DIR))
                                SRC_LIST += $(CVAR_DIR)/cvar.c
                                SRC_LIST += $(CVAR_DIR)/cvar_q1.c


                        $(eval CMD_DIR = $(CLI_DIR)/cmd) $(eval INCLUDES += $(CMD_DIR))
                                $(eval CBUF_DIR = $(CMD_DIR)/Cbuf) $(eval INCLUDES += $(CBUF_DIR))
                                        #SRC_LIST += $(CBUF_DIR)/cbuf.c
                                        SRC_LIST += $(CBUF_DIR)/cbuf_cWrap.cpp
                                        SRC_LIST += $(CBUF_DIR)/cbuf_obj.cpp

                                SRC_LIST += $(CMD_DIR)/cmd.c
                                SRC_LIST += $(CMD_DIR)/cmd_alias.c

                        SRC_LIST += $(CLI_DIR)/console.c


                $(eval COMM_DIR = $(HOST_DIR)/common) $(eval INCLUDES += $(COMM_DIR))
                        SRC_LIST += $(COMM_DIR)/common.c


                $(eval INPUT_DIR = $(HOST_DIR)/input) $(eval INCLUDES += $(INPUT_DIR))
                        SRC_LIST += $(INPUT_DIR)/keys.c




        $(eval SV_SIDE_DIR = $(ENG_DIR)/Server_side) $(eval INCLUDES += $(SV_SIDE_DIR))
                $(eval WORLD_DIR = $(SV_SIDE_DIR)/world) $(eval INCLUDES += $(WORLD_DIR))
                        SRC_LIST += $(WORLD_DIR)/world.c

                $(eval PROG_DIR = $(SV_SIDE_DIR)/qcvm) $(eval INCLUDES += $(PROG_DIR))
                        SRC_LIST += $(PROG_DIR)/pr_cmds.c
                        SRC_LIST += $(PROG_DIR)/pr_edict.c
                        SRC_LIST += $(PROG_DIR)/pr_exec.c

                SRC_LIST += $(SV_SIDE_DIR)/sv_main.c
                SRC_LIST += $(SV_SIDE_DIR)/sv_user.c
                SRC_LIST += $(SV_SIDE_DIR)/sv_phys.c
                SRC_LIST += $(SV_SIDE_DIR)/sv_move.c

        $(eval CL_SIDE_DIR = $(ENG_DIR)/Client_side) $(eval INCLUDES += $(CL_SIDE_DIR))
                $(eval SND_DIR = $(CL_SIDE_DIR)/sound) $(eval INCLUDES += $(SND_DIR))
                        SRC_LIST += $(SND_DIR)/snd_dma.c
                        SRC_LIST += $(SND_DIR)/snd_mem.c
                        SRC_LIST += $(SND_DIR)/snd_mix.c

                SRC_LIST += $(CL_SIDE_DIR)/cl_main.c
                SRC_LIST += $(CL_SIDE_DIR)/cl_input.c
                SRC_LIST += $(CL_SIDE_DIR)/cl_demo.c
                SRC_LIST += $(CL_SIDE_DIR)/cl_parse.c
                SRC_LIST += $(CL_SIDE_DIR)/cl_tent.c

                $(eval VID_DIR = $(CL_SIDE_DIR)/video) $(eval INCLUDES += $(VID_DIR))
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

                        $(eval UI_DIR = $(CL_SIDE_DIR)/ui) $(eval INCLUDES += $(UI_DIR))
                                SRC_LIST += $(UI_DIR)/screen.c
                                SRC_LIST += $(UI_DIR)/view.c



        $(eval SHARED_DIR = $(ENG_DIR)/Shared) $(eval INCLUDES += $(SHARED_DIR))
                $(eval NET_DIR = $(SHARED_DIR)/Net) $(eval INCLUDES += $(NET_DIR))
                        $(eval MSG_DIR = $(NET_DIR)/Msg) $(eval INCLUDES += $(MSG_DIR))
                                #SRC_LIST += $(MSG_DIR)/msg.c
                                SRC_LIST += $(MSG_DIR)/msg_cWrap.cpp
                                SRC_LIST += $(MSG_DIR)/msg_obj.cpp


                        SRC_LIST += $(NET_DIR)/net_main.c
                        SRC_LIST += $(NET_DIR)/net_dgrm.c
                        SRC_LIST += $(NET_DIR)/net_loop.c
                        SRC_LIST += $(NET_DIR)/net_vcr.c
                $(eval MATH_DIR = $(SHARED_DIR)/math) $(eval INCLUDES += $(MATH_DIR))
                        SRC_LIST += $(MATH_DIR)/mathlib.c
                        SRC_LIST += $(MATH_DIR)/Vector3d.cpp
                        SRC_LIST += $(MATH_DIR)/vector.cpp
                        SRC_LIST += $(MATH_DIR)/angle.c

                $(eval AST_DIR = $(SHARED_DIR)/assets) $(eval INCLUDES += $(AST_DIR))
                        $(eval MDL_DIR = $(AST_DIR)/model) $(eval INCLUDES += $(MDL_DIR))
                                SRC_LIST += $(MDL_DIR)/model.c

                        $(eval WAD_DIR = $(AST_DIR)/wad) $(eval INCLUDES += $(WAD_DIR))
                                SRC_LIST += $(WAD_DIR)/wad.c

                $(eval STRUCT_DIR := $(SHARED_DIR)/structs) $(eval INCLUDES += $(STRUCT_DIR))
                        SRC_LIST += $(STRUCT_DIR)/qPic.c
                        SRC_LIST += $(STRUCT_DIR)/pcx.c
                        SRC_LIST += $(STRUCT_DIR)/Pak.c


                $(eval CUTILS_DIR = $(SHARED_DIR)/utils) $(eval INCLUDES += $(CUTILS_DIR))
                        SRC_LIST += $(CUTILS_DIR)/crc.c
                        SRC_LIST += $(CUTILS_DIR)/endian_tools.c
                        SRC_LIST += $(CUTILS_DIR)/q_tools.c
                        SRC_LIST += $(CUTILS_DIR)/link.c

                $(eval ZONE_DIR = $(SHARED_DIR)/zone) $(eval INCLUDES += $(ZONE_DIR))
                        SRC_LIST += $(ZONE_DIR)/zone.c
                        SRC_LIST += $(ZONE_DIR)/z_hulk.c
                        SRC_LIST += $(ZONE_DIR)/z_cache.c
                        SRC_LIST += $(ZONE_DIR)/z_zone.c

                $(eval SBUF_DIR = $(SHARED_DIR)/sBuf) $(eval INCLUDES += $(SBUF_DIR))
                        SRC_LIST += $(SBUF_DIR)/sizebuf.c


$(eval GAME_DIR := $(SRC_DIR)/game/Quake) $(eval INCLUDES += $(GAME_DIR))
        $(eval MENU_DIR = $(GAME_DIR)/menu) $(eval INCLUDES += $(MENU_DIR))
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

        SRC_LIST += $(GAME_DIR)/game_rule.c
        SRC_LIST += $(GAME_DIR)/sbar.c
        SRC_LIST += $(GAME_DIR)/chase.c
