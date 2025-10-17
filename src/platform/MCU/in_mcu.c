#include "client.h"

void IN_Init() {}

void IN_Shutdown() {}

// oportunity for devices to stick commands on the script buffer
void IN_Commands() {}

// add additional movement on top of the keyboard move cmd
void IN_Move(UserCmd_p cmd) {}

// restores all button and position states to defaults
void IN_ClearStates() {}