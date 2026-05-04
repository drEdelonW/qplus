DEFINES += GLQUAKE
CFLAGS  += -I/opt/X11/include
LDFLAGS += -L/opt/X11/lib -lGL -framework OpenGL

                $(eval SOFTRND_DIR = $(CL_SIDE_DIR)/video/soft) $(eval INCLUDES += $(SOFTRND_DIR))
                        $(eval DRAW_DIR = $(SOFTRND_DIR)/draw2D) $(eval INCLUDES += $(DRAW_DIR))
#                                 SRC_LIST += $(DRAW_DIR)/nonintel.c
#                                 SRC_LIST += $(DRAW_DIR)/d_edge.c
#                                 SRC_LIST += $(DRAW_DIR)/d_fill.c
#                                 SRC_LIST += $(DRAW_DIR)/d_init.c
#                                 SRC_LIST += $(DRAW_DIR)/d_modech.c
#                                 SRC_LIST += $(DRAW_DIR)/d_part.c
#                                 SRC_LIST += $(DRAW_DIR)/d_polyse.c
#                                 SRC_LIST += $(DRAW_DIR)/d_scan.c
#                                 SRC_LIST += $(DRAW_DIR)/d_sky.c
#                                 SRC_LIST += $(DRAW_DIR)/d_sprite.c
#                                 SRC_LIST += $(DRAW_DIR)/d_surf.c
                                SRC_LIST += $(DRAW_DIR)/d_vars.c
#                                 SRC_LIST += $(DRAW_DIR)/d_zpoint.c

                        $(eval RENDER_DIR = $(SOFTRND_DIR)/render3D) $(eval INCLUDES += $(RENDER_DIR))
#                                 SRC_LIST += $(RENDER_DIR)/r_aclip.c
#                                 SRC_LIST += $(RENDER_DIR)/r_alias.c
#                                 SRC_LIST += $(RENDER_DIR)/r_bsp.c
#                                 SRC_LIST += $(RENDER_DIR)/r_edge.c
#                                 SRC_LIST += $(RENDER_DIR)/r_sky.c
#                                 SRC_LIST += $(RENDER_DIR)/r_sprite.c
                                SRC_LIST += $(RENDER_DIR)/r_part.c
#                                 SRC_LIST += $(RENDER_DIR)/r_vars.c

                $(eval VID_DIR = $(CL_SIDE_DIR)/video) $(eval INCLUDES += $(VID_DIR))
                        $(eval OPENGL_DIR = $(VID_DIR)/gl) $(eval INCLUDES += $(OPENGL_DIR))
#                                 SRC_LIST += $(OPENGL_DIR)/gl_vidlinux.c
#                                 SRC_LIST += $(OPENGL_DIR)/gl_vidlinuxglx.c
                                SRC_LIST += $(OPENGL_DIR)/vid_x11gl.c

                                SRC_LIST += $(OPENGL_DIR)/gl_screen.c
#                                 SRC_LIST += $(RENDER_DIR)/r_draw.c
                                SRC_LIST += $(OPENGL_DIR)/gl_draw.c
#                                 SRC_LIST += $(RENDER_DIR)/r_main.c
                                SRC_LIST += $(OPENGL_DIR)/gl_rmain.c
#                                 SRC_LIST += $(RENDER_DIR)/r_misc.c
                                SRC_LIST += $(OPENGL_DIR)/gl_rmisc.c
#                                 SRC_LIST += $(RENDER_DIR)/r_surf.c
                                SRC_LIST += $(OPENGL_DIR)/gl_rsurf.c
#                                 SRC_LIST += $(RENDER_DIR)/r_light.c
                                SRC_LIST += $(OPENGL_DIR)/gl_rlight.c
#                                 SRC_LIST += $(RENDER_DIR)/r_efrag.c
                                SRC_LIST += $(OPENGL_DIR)/gl_refrag.c
                                SRC_LIST += $(OPENGL_DIR)/gl_warp.c
                                SRC_LIST += $(OPENGL_DIR)/gl_model.c
                                SRC_LIST += $(OPENGL_DIR)/gl_mesh.c

        $(eval SHARED_DIR = $(ENG_DIR)/Shared) $(eval INCLUDES += $(SHARED_DIR))
                $(eval AST_DIR = $(SHARED_DIR)/assets) $(eval INCLUDES += $(AST_DIR))
                        $(eval MDL_DIR = $(AST_DIR)/model) $(eval INCLUDES += $(MDL_DIR))