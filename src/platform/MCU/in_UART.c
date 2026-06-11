#include "input.h"

void Sys_SendKeyEvents() {} // keys handle

void IN_Init() {}

void IN_Shutdown() {}

void IN_Commands() {}   // allow mice or other external controllers to add commands

void IN_Move(UserCmd_p cmd) {// allow mice or other external controllers to add to the move
#if 0
    if (ActiveApp && !Minimized) {
        IN_MouseMove(cmd);
        IN_JoyMove(cmd);
    }
#endif
}  
