
INCLUDES += $(SRC_DIR)

NET_DIR = $(SRC_DIR)/net
 INCLUDES += $(NET_DIR)
  SRC_LIST += $(NET_DIR)/net_dgrm.c
  SRC_LIST += $(NET_DIR)/net_loop.c
  SRC_LIST += $(NET_DIR)/net_main.c
  SRC_LIST += $(NET_DIR)/net_vcr.c
  SRC_LIST += $(NET_DIR)/net_udp.c
  SRC_LIST += $(NET_DIR)/net_bsd.c

NET_CL_DIR = $(NET_DIR)/client
 INCLUDES += $(NET_CL_DIR)
  SRC_LIST += $(NET_CL_DIR)/cl_demo.c
  SRC_LIST += $(NET_CL_DIR)/cl_input.c
  SRC_LIST += $(NET_CL_DIR)/cl_main.c
  SRC_LIST += $(NET_CL_DIR)/cl_parse.c
  SRC_LIST += $(NET_CL_DIR)/cl_tent.c

NET_SV_DIR = $(NET_DIR)/server
 INCLUDES += $(NET_SV_DIR)
  SRC_LIST += $(NET_SV_DIR)/sv_main.c
  SRC_LIST += $(NET_SV_DIR)/sv_phys.c
  SRC_LIST += $(NET_SV_DIR)/sv_move.c
  SRC_LIST += $(NET_SV_DIR)/sv_user.c


VID_DIR = $(SRC_DIR)/vid
 INCLUDES += $(VID_DIR)
  SRC_LIST += $(VID_DIR)/vid_x.c

DRAW_DIR = $(VID_DIR)/draw
 INCLUDES += $(DRAW_DIR)
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

RENDER_DIR = $(VID_DIR)/render
 INCLUDES += $(RENDER_DIR)
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

SND_DIR = $(SRC_DIR)/sound
 INCLUDES += $(SND_DIR)
  SRC_LIST += $(SND_DIR)/snd_dma.c
  SRC_LIST += $(SND_DIR)/snd_mem.c
  SRC_LIST += $(SND_DIR)/snd_mix.c
  SRC_LIST += $(SND_DIR)/snd_linux.c

PROG_DIR = $(SRC_DIR)/prog_Qc
 INCLUDES += $(PROG_DIR)
  SRC_LIST += $(PROG_DIR)/pr_cmds.c
  SRC_LIST += $(PROG_DIR)/pr_edict.c
  SRC_LIST += $(PROG_DIR)/pr_exec.c

MATH_DIR = $(SRC_DIR)/math
 INCLUDES += $(MATH_DIR)
  SRC_LIST += $(MATH_DIR)/mathlib.c

CLI_DIR = $(SRC_DIR)/console
 INCLUDES += $(CLI_DIR)
  SRC_LIST += $(CLI_DIR)/console.c
  SRC_LIST += $(CLI_DIR)/cvar.c

OTHER_DIR = $(SRC_DIR)/etc
 INCLUDES += $(OTHER_DIR)
  SRC_LIST += $(OTHER_DIR)/chase.c
  SRC_LIST += $(OTHER_DIR)/cmd.c
  SRC_LIST += $(OTHER_DIR)/common.c
  SRC_LIST += $(OTHER_DIR)/crc.c

  SRC_LIST += $(OTHER_DIR)/host.c
  SRC_LIST += $(OTHER_DIR)/host_cmd.c
  SRC_LIST += $(OTHER_DIR)/keys.c
  SRC_LIST += $(OTHER_DIR)/menu.c
  SRC_LIST += $(OTHER_DIR)/model.c
  SRC_LIST += $(OTHER_DIR)/nonintel.c

  SRC_LIST += $(OTHER_DIR)/screen.c
  SRC_LIST += $(OTHER_DIR)/sbar.c
  SRC_LIST += $(OTHER_DIR)/zone.c
  SRC_LIST += $(OTHER_DIR)/view.c
  SRC_LIST += $(OTHER_DIR)/wad.c
  SRC_LIST += $(OTHER_DIR)/world.c

  SRC_LIST += $(OTHER_DIR)/cd_linux.c

  SRC_LIST += $(OTHER_DIR)/sys_linux.c

