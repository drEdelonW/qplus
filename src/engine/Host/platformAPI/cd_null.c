// cd_null.c — stub for non-Linux builds

#include "cdaudio.h"
#include "mem_placement.h"

WEAK int   CDAudio_Init() { return 0; }
WEAK void  CDAudio_Shutdown() {}
WEAK void  CDAudio_Play(uint8_t track, bool looping) {}
WEAK void  CDAudio_Stop() {}
WEAK void  CDAudio_Pause() {}
WEAK void  CDAudio_Resume() {}
WEAK void  CDAudio_Update() {}

