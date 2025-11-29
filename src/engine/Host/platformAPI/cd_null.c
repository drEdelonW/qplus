// cd_null.c — stub for non-Linux builds

#include "cdaudio.h"
#include "mem_placement.h"

__weak int   CDAudio_Init() { return 0; }
__weak void  CDAudio_Shutdown() {}
__weak void  CDAudio_Play(uint8_t track, bool looping) {}
__weak void  CDAudio_Stop() {}
__weak void  CDAudio_Pause() {}
__weak void  CDAudio_Resume() {}
__weak void  CDAudio_Update() {}

