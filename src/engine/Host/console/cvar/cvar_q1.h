#pragma once

#include "cvar.h"

// FROM: src/platform/legacy/vid_svgalib.c
    // extern cvar_t  mouse_button_commands[];

// FROM: src/platform/posix/vid_x.c
CVAR_EXTERN(_windowed_mouse);
CVAR_EXTERN(m_filter);

// FROM: src/engine/core/host_cmd.c
CVAR_EXTERN(pausable);

// FROM: src/engine/core/host.h
CVAR_EXTERN(sys_ticrate);
CVAR_EXTERN(sys_nostdout);
CVAR_EXTERN(developer);

// FROM: src/engine/core/host.c
CVAR_EXTERN(host_framerate);
CVAR_EXTERN(host_speeds);
CVAR_EXTERN(serverprofile);

CVAR_EXTERN(samelevel);
CVAR_EXTERN(noexit);

CVAR_EXTERN(temp1);


// FROM: src/engine/core/common/common.c
CVAR_EXTERN(registered);
CVAR_EXTERN(cmdline);


// FROM: src/engine/core/console/console.c
CVAR_EXTERN(con_notifytime);		//seconds

// FROM: src/engine/net/net_main.c
CVAR_EXTERN(net_messagetimeout);
CVAR_EXTERN(hostname);

CVAR_EXTERN(config_com_port);
CVAR_EXTERN(config_com_irq);
CVAR_EXTERN(config_com_baud);
CVAR_EXTERN(config_com_modem);
CVAR_EXTERN(config_modem_dialtype);
CVAR_EXTERN(config_modem_clear);
CVAR_EXTERN(config_modem_init);
CVAR_EXTERN(config_modem_hangup);

#ifdef IDGODS
CVAR_EXTERN(idgods);
#endif

// FROM: src/engine/net/client/cl_main.c
    // these two are not intended to be set directly
CVAR_EXTERN(cl_name);
CVAR_EXTERN(cl_color);

CVAR_EXTERN(cl_shownet);	// can be 0, 1, or 2
CVAR_EXTERN(cl_nolerp);

CVAR_EXTERN(lookspring);
CVAR_EXTERN(lookstrafe);
CVAR_EXTERN(sensitivity);

CVAR_EXTERN(m_pitch);
CVAR_EXTERN(m_yaw);
CVAR_EXTERN(m_forward);
CVAR_EXTERN(m_side);

// FROM: src/engine/net/client/client.h
    //
    // cvars
    //
CVAR_EXTERN(cl_upspeed);
CVAR_EXTERN(cl_forwardspeed);
CVAR_EXTERN(cl_backspeed);
CVAR_EXTERN(cl_sidespeed);

CVAR_EXTERN(cl_movespeedkey);

CVAR_EXTERN(cl_yawspeed);
CVAR_EXTERN(cl_pitchspeed);

CVAR_EXTERN(cl_anglespeedkey);

CVAR_EXTERN(cl_autofire);

CVAR_EXTERN(cl_pitchdriftspeed);

// FROM: src/engine/net/server/server.h
CVAR_EXTERN(teamplay);
CVAR_EXTERN(skill);
CVAR_EXTERN(deathmatch);
CVAR_EXTERN(coop);
CVAR_EXTERN(fraglimit);
CVAR_EXTERN(timelimit);


// FROM: src/engine/net/server/sv_main.c
CVAR_EXTERN(sv_maxvelocity);
CVAR_EXTERN(sv_gravity);
CVAR_EXTERN(sv_nostep);
CVAR_EXTERN(sv_friction);
CVAR_EXTERN(sv_edgefriction);
CVAR_EXTERN(sv_stopspeed);
CVAR_EXTERN(sv_maxspeed);
CVAR_EXTERN(sv_accelerate);
CVAR_EXTERN(sv_idealpitchscale);
CVAR_EXTERN(sv_aim);

// FROM: src/engine/vm/pr_edict.c
CVAR_EXTERN(nomonsters);
CVAR_EXTERN(gamecfg);
CVAR_EXTERN(scratch1);
CVAR_EXTERN(scratch2);
CVAR_EXTERN(scratch3);
CVAR_EXTERN(scratch4);
CVAR_EXTERN(savedgamecfg);
CVAR_EXTERN(saved1);
CVAR_EXTERN(saved2);
CVAR_EXTERN(saved3);
CVAR_EXTERN(saved4);

// FROM: src/video/render/r_main.c
CVAR_EXTERN(scr_fov);

// FROM: src/video/render/r_local.h
CVAR_EXTERN(r_draworder);
CVAR_EXTERN(r_speeds);
CVAR_EXTERN(r_timegraph);
CVAR_EXTERN(r_graphheight);
CVAR_EXTERN(r_clearcolor);
CVAR_EXTERN(r_waterwarp);
CVAR_EXTERN(r_fullbright);
CVAR_EXTERN(r_drawentities);
CVAR_EXTERN(r_drawviewmodel);
CVAR_EXTERN(r_drawworld);
CVAR_EXTERN(r_lightmap);
CVAR_EXTERN(r_dlightmap);
CVAR_EXTERN(r_aliasstats);
CVAR_EXTERN(r_dspeeds);
CVAR_EXTERN(r_drawflat);
CVAR_EXTERN(r_ambient);
CVAR_EXTERN(r_reportsurfout);
CVAR_EXTERN(r_maxsurfs);
CVAR_EXTERN(r_numsurfs);
CVAR_EXTERN(r_reportedgeout);
CVAR_EXTERN(r_maxedges);
CVAR_EXTERN(r_numedges);
CVAR_EXTERN(r_aliastransbase);
CVAR_EXTERN(r_aliastransadj);

// FROM: src/ui/view.c
CVAR_EXTERN(lcd_x);
CVAR_EXTERN(lcd_yaw);

CVAR_EXTERN(scr_ofsx);
CVAR_EXTERN(scr_ofsy);
CVAR_EXTERN(scr_ofsz);

CVAR_EXTERN(cl_rollspeed);
CVAR_EXTERN(cl_rollangle);

CVAR_EXTERN(cl_bob);
CVAR_EXTERN(cl_bobcycle);
CVAR_EXTERN(cl_bobup);

CVAR_EXTERN(v_kicktime);
CVAR_EXTERN(v_kickroll);
CVAR_EXTERN(v_kickpitch);

CVAR_EXTERN(v_iyaw_cycle);
CVAR_EXTERN(v_iroll_cycle);
CVAR_EXTERN(v_ipitch_cycle);
CVAR_EXTERN(v_iyaw_level);
CVAR_EXTERN(v_iroll_level);
CVAR_EXTERN(v_ipitch_level);

CVAR_EXTERN(v_idlescale);

CVAR_EXTERN(crosshair);
CVAR_EXTERN(cl_crossx);
CVAR_EXTERN(cl_crossy);

CVAR_EXTERN(gl_cshiftpercent);


// FROM: src/sound/snd_dma.c
CVAR_EXTERN(bgmvolume);
CVAR_EXTERN(volume);

CVAR_EXTERN(nosound);
CVAR_EXTERN(precache);
CVAR_EXTERN(loadas8bit);
CVAR_EXTERN(bgmbuffer);
CVAR_EXTERN(ambient_level);
CVAR_EXTERN(ambient_fade);
CVAR_EXTERN(snd_noextraupdate);
CVAR_EXTERN(snd_show);
CVAR_EXTERN(_snd_mixahead);


// FROM: src/ui/screen.c
CVAR_EXTERN(scr_viewsize);
CVAR_EXTERN(scr_fov);	// 10 - 170
CVAR_EXTERN(scr_conspeed);
CVAR_EXTERN(scr_centertime);
CVAR_EXTERN(scr_showram);
CVAR_EXTERN(scr_showturtle);
CVAR_EXTERN(scr_showpause);
CVAR_EXTERN(scr_printspeed);

// FROM: src/ui/screen.c
CVAR_EXTERN(d_subdiv16);
CVAR_EXTERN(d_mipcap);
CVAR_EXTERN(d_mipscale);

// FROM: src/ui/chase.c
CVAR_EXTERN(chase_back);
CVAR_EXTERN(chase_up);
CVAR_EXTERN(chase_right);
CVAR_EXTERN(chase_active);

// FROM: src/ui/view.c
CVAR_EXTERN(v_gamma);

CVAR_EXTERN(v_centermove);
CVAR_EXTERN(v_centerspeed);