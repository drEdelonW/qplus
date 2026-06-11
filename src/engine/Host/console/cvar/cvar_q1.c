#include "cvar_q1.h"

// FROM: src/platform/legacy/vid_svgalib.c
    // cvar_t  mouse_button_commands[3] = {
    //     {"mouse1", "+attack"},
    //     {"mouse2", "+strafe"},
    //     {"mouse3", "+forward"},
    // };

    // CVAR(vid_mode, "5");
    // CVAR(vid_redrawfull, "0");
    // CVAR_ARC(vid_waitforrefresh, "0");

    // CVAR(m_filter, "0");

CVAR_ARC(_windowed_mouse, "0");
CVAR_ARC(m_filter, "0");

// FROM: src/engine/core/host.c
CVAR(host_framerate, "0");  // set for slow motion
CVAR(host_speeds, "0");     // set for running times

CVAR(sys_ticrate, "0.05");
CVAR(serverprofile, "0");

CVAR_SV(fraglimit, "0");
CVAR_SV(timelimit, "0");
CVAR_SV(teamplay, "0");

CVAR(samelevel, "0");
CVAR_SV(noexit, "0");

#ifdef QUAKE2
CVAR(developer, "1");   // should be 0 for release!
#else
// CVAR(developer, "0");
CVAR(developer, "1");
#endif

CVAR(skill, "1");       // 0 - 3
CVAR(deathmatch, "0");  // 0, 1, or 2
CVAR(coop, "0");        // 0 or 1

CVAR(pausable, "1");

CVAR(temp1, "0");

// FROM: src/engine/core/common/common.c
CVAR(registered, "0");
CVAR_SV(cmdline, "0");

// FROM: src/engine/core/console/console.c
CVAR(con_notifytime, "3");		//seconds


// FROM: src/engine/net/net_main.c
CVAR(net_messagetimeout, "300");
CVAR(hostname, "UNNAMED");

CVAR_CFG(config_com_port, "0x3f8");
CVAR_CFG(config_com_irq, "4");
CVAR_CFG(config_com_baud, "57600");
CVAR_CFG(config_com_modem, "1");
CVAR_CFG(config_modem_dialtype, "T");
CVAR_CFG(config_modem_clear, "ATZ");
CVAR_CFG(config_modem_init, "");
CVAR_CFG(config_modem_hangup, "AT H");

#ifdef IDGODS
CVAR(idgods, "0");
#endif


// FROM: src/engine/net/client/cl_input.c
CVAR(cl_upspeed, "200");
CVAR_ARC(cl_forwardspeed, "200");
CVAR_ARC(cl_backspeed, "200");
CVAR(cl_sidespeed, "350");

CVAR(cl_movespeedkey, "2.0");

CVAR(cl_yawspeed, "140");
CVAR(cl_pitchspeed, "150");

CVAR(cl_anglespeedkey, "1.5");


// FROM: src/engine/net/client/cl_main.c
    // these two are not intended to be set directly
CVAR_CFG(cl_name, "player");
CVAR_CFG(cl_color, "0");

CVAR(cl_shownet, "0");	// can be 0, 1, or 2
CVAR(cl_nolerp, "0");

CVAR_ARC(lookspring, "0");
CVAR_ARC(lookstrafe, "0");
CVAR_ARC(sensitivity, "3");

CVAR_ARC(m_pitch, "0.022");
CVAR_ARC(m_yaw, "0.022");
CVAR_ARC(m_forward, "1");
CVAR_ARC(m_side, "0.8");

// FROM: src/engine/net/server/sv_phys.c
CVAR_SV(sv_friction, "4");
CVAR(sv_stopspeed, "100");
CVAR_SV(sv_gravity, "800");
CVAR(sv_maxvelocity, "2000");
CVAR(sv_nostep, "0");

// FROM: src/engine/net/server/sv_user.c
CVAR_EXTERN(sv_friction);
CVAR_NAMED(sv_edgefriction, "edgefriction", "2");
CVAR_EXTERN(sv_stopspeed);

CVAR(sv_idealpitchscale, "0.8");

CVAR_SV(sv_maxspeed, "320");
CVAR(sv_accelerate, "10");

// FROM: src/engine/vm/pr_cmds.c
CVAR(sv_aim, "0.93");

// FROM: src/engine/vm/pr_edict.c
CVAR(nomonsters, "0");
CVAR(gamecfg, "0");
CVAR(scratch1, "0");
CVAR(scratch2, "0");
CVAR(scratch3, "0");
CVAR(scratch4, "0");
CVAR_ARC(savedgamecfg, "0");
CVAR_ARC(saved1, "0");
CVAR_ARC(saved2, "0");
CVAR_ARC(saved3, "0");
CVAR_ARC(saved4, "0");

// FROM: src/video/render/r_main.c
CVAR(r_draworder, "0");
CVAR(r_speeds, "0");
CVAR(r_timegraph, "0");
CVAR(r_graphheight, "10");
CVAR(r_clearcolor, "2");
CVAR(r_waterwarp, "1");
CVAR(r_fullbright, "0");
CVAR(r_drawentities, "1");
CVAR(r_drawviewmodel, "1");
CVAR(r_drawworld, "1");
CVAR(r_lightmap, "1");
#ifdef STM32
/* */CVAR_ARC(r_dlightmap, "0");
#else
/* */CVAR_ARC(r_dlightmap, "1");
#endif
CVAR_NAMED(r_aliasstats, "r_polymodelstats", "0");
CVAR(r_dspeeds, "0");
CVAR_ARC(r_drawflat, "0");
CVAR(r_ambient, "0");
CVAR(r_reportsurfout, "0");
CVAR(r_maxsurfs, "0");
CVAR(r_numsurfs, "0");
CVAR(r_reportedgeout, "0");
CVAR(r_maxedges, "0");
CVAR(r_numedges, "0");
CVAR(r_aliastransbase, "200");
CVAR(r_aliastransadj, "100");

// FROM: src/ui/view.c
CVAR(lcd_x, "0");
CVAR(lcd_yaw, "0");

CVAR(scr_ofsx, "0");
CVAR(scr_ofsy, "0");
CVAR(scr_ofsz, "0");

CVAR(cl_rollspeed, "200");
CVAR(cl_rollangle, "2.0");

CVAR(cl_bob, "0.02");
CVAR(cl_bobcycle, "0.6");
CVAR(cl_bobup, "0.5");

CVAR(v_kicktime, "0.5");
CVAR(v_kickroll, "0.6");
CVAR(v_kickpitch, "0.6");

CVAR(v_iyaw_cycle, "2");
CVAR(v_iroll_cycle, "0.5");
CVAR(v_ipitch_cycle, "1");
CVAR(v_iyaw_level, "0.3");
CVAR(v_iroll_level, "0.1");
CVAR(v_ipitch_level, "0.3");

CVAR(v_idlescale, "0");

CVAR_ARC(crosshair, "0");
CVAR(cl_crossx, "0");
CVAR(cl_crossy, "0");

CVAR(gl_cshiftpercent, "100");

// FROM: src/sound/snd_dma.c
CVAR_ARC(bgmvolume, "1");
CVAR_ARC(volume, "0.7");

CVAR(nosound, "0");
CVAR(precache, "1");
CVAR(loadas8bit, "0");
CVAR(bgmbuffer, "4096");
CVAR(ambient_level, "0.3");
CVAR(ambient_fade, "100");
CVAR(snd_noextraupdate, "0");
CVAR(snd_show, "0");
CVAR_ARC(_snd_mixahead, "0.1");

// FROM: src/ui/screen.c
CVAR_NAMED_AR(scr_viewsize, "viewsize", "100");
CVAR_NAMED(scr_fov, "fov", "90");	// 10 - 170
CVAR(scr_conspeed, "300");
CVAR(scr_centertime, "2");
CVAR_NAMED(scr_showram, "showram", "1");
CVAR_NAMED(scr_showturtle, "showturtle", "0");
CVAR_NAMED(scr_showpause, "showpause", "1");
CVAR(scr_printspeed, "8");

// FROM: src/ui/screen.c
CVAR(d_subdiv16, "1");
#ifdef STM32
/* */CVAR_ARC(d_mipcap, "3");
#else
/* */CVAR_ARC(d_mipcap, "0");
#endif
CVAR(d_mipscale, "1");

// FROM: src/ui/chase.c
CVAR(chase_back, "100");
CVAR(chase_up, "16");
CVAR(chase_right, "0");
CVAR(chase_active, "0");

// FROM: src/ui/view.c
CVAR_NAMED_AR(v_gamma, "gamma", "1");

CVAR(v_centermove, "0.15");
CVAR(v_centerspeed, "500");
