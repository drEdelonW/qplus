#pragma once

/*
=============================================================================
pr_builtins_priv.h - private declarations for pr_builtins.c only
=============================================================================
*/
#pragma once

// pf_print.c
void PF_VarString_init();
void PF_error();
void PF_objerror();
void PF_bprint();
void PF_sprint();
void PF_centerprint();
void PF_dprint();
void PF_ftos();
void PF_vtos();
void PF_break();
#ifdef QUAKE2
void PF_etos();
#endif

// pf_math.c
void PF_makevectors();
void PF_normalize();
void PF_vlen();
void PF_vectoyaw();
void PF_vectoangles();
void PF_random();
void PF_rint();
void PF_floor();
void PF_ceil();
void PF_fabs();
#ifdef QUAKE2
void PF_sin();
void PF_cos();
void PF_sqrt();
#endif

// pf_edict.c
void PF_setorigin();
void PF_setsize();
void PF_setmodel();
void PF_Spawn();
void PF_Remove();
void PF_Find();
void PF_findradius();
void PF_nextent();
void PF_makestatic();
void PF_setspawnparms();
void PF_checkclient();

// pf_move.c
void PF_traceline();
void PF_walkmove();
void PF_droptofloor();
void PF_checkbottom();
void PF_aim();
void PF_changeyaw();
void PF_pointcontents();
#ifdef QUAKE2
void PF_TraceToss();
void PF_changepitch();
void PF_WaterMove();
#endif

// pf_sound.c
void PF_sound();
void PF_ambientsound();

// pf_msg.c
void PF_WriteByte();
void PF_WriteChar();
void PF_WriteShort();
void PF_WriteLong();
void PF_WriteAngle();
void PF_WriteCoord();
void PF_WriteString();
void PF_WriteEntity();

// pf_world.c
void PF_lightstyle();
void PF_particle();
void PF_precache_sound();
void PF_precache_model();
void PF_precache_file();
void PF_changelevel();

// pf_cvar.c
void PF_stuffcmd();
void PF_localcmd();
void PF_cvar();
void PF_cvar_set();

// pf_debug.c
void PF_coredump();
void PF_traceon();
void PF_traceoff();
void PF_eprint();
void PF_checkpos();

// pr_builtins.c
void PF_Fixme();