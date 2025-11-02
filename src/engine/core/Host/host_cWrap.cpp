#include "host.h"
#include "host.hpp"

/*
================
Host_FindMaxClients
================
*/
void Host_FindMaxClients() {
    host.FindMaxClients();
}


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal() {
    host.InitLocal();
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration() {
    host.WriteConfiguration();
}




/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(bool crash) {
    host.ShutdownServer(crash);
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory() {
    host.ClearMemory();
}


//============================================================================


/*
===================
Host_FilterTime

Returns false if the time is too int16_t to run a frame
===================
*/
bool Host_FilterTime(float time) {
    return host.FilterTime(time);
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands() {
    host.GetConsoleCommands();
}


/*
==================
Host_ServerFrame

==================
*/

void Host_ServerFrame() {
    host.ServerFrame();
}


/*
==================
Host_Frame

Runs all active servers
==================
*/

void Host_Frame(float time) {
    host.Frame(time);
}

//============================================================================


void Host_InitVCR(QuakeParms_p parms) {
    host.InitVCR(parms);
}

/*
====================
Host_Init
====================
*/
void Host_Init(QuakeParms_p parms) {
    host.Init(parms);
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown() {
  host.Shutdown();
}

