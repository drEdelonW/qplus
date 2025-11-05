#include "host.h"
#include "host.hpp"

void Host_ShutdownServer(bool crash) {
    host.ShutdownServer(crash);
}

void Host_ClearMemory() {
    host.ClearMemory();
}

void Host_Frame(float time) {
    host.Frame(time);
}

void Host_Init(QuakeParms_p parms) {
    host.Init(parms);
}

void Host_Shutdown() {
    host.Shutdown();
}
