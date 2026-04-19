#include "host.h"
#include "host.hpp"
#include "sys.h"

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

bool Host_IsDedicated() {
    return host.IsDedicated();
}

bool Host_IsServerActive() {
    return host.IsServerActive();
}

double Host_FloatTime() {
    return Sys_FloatTime();
}
