#pragma once

#include "qparams.h"

//
// host
//
extern	quakeparms_t host_parms;

#include "cvar_q1.h"

#include "qboolean.h"
extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;

#include "byte_t.h"
extern	byte		*host_basepal;
extern	byte		*host_colormap;
extern	int			host_framecount;	// incremented every frame, never reset
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset

void Host_ClearMemory (void);
void Host_ServerFrame (void);
void Host_InitCommands (void);
void Host_Init (quakeparms_t *parms);
void Host_Shutdown(void);
void Host_Error (char *error, ...);
void Host_EndGame (char *message, ...);
void Host_Frame (float time);
void Host_Quit_f (void);
void Host_ClientCommands (char *fmt, ...);
void Host_ShutdownServer (qboolean crash);
