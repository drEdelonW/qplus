#include "host.h"
#include "host.hpp"


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
==================
Host_Frame

Runs all active servers
==================
*/

void Host_Frame(float time) {
    host.Frame(time);
}

//============================================================================


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

// void Host_InitCommands() {
//     host.InitCommands();
// }