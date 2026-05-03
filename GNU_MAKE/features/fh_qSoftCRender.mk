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

        $(eval SHARED_DIR = $(ENG_DIR)/Shared) $(eval INCLUDES += $(SHARED_DIR))
                $(eval AST_DIR = $(SHARED_DIR)/assets) $(eval INCLUDES += $(AST_DIR))
                        $(eval MDL_DIR = $(AST_DIR)/model) $(eval INCLUDES += $(MDL_DIR))
                                SRC_LIST += $(MDL_DIR)/model.c