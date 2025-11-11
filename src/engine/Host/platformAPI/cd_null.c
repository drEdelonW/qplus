// cd_null.c — stub for non-Linux builds

#include "cdaudio.h"

__attribute__((weak)) int   CDAudio_Init() { return 0; }
__attribute__((weak)) void  CDAudio_Shutdown() {}
__attribute__((weak)) void  CDAudio_Play(uint8_t track, bool looping) {}
__attribute__((weak)) void  CDAudio_Stop() {}
__attribute__((weak)) void  CDAudio_Pause() {}
__attribute__((weak)) void  CDAudio_Resume() {}
__attribute__((weak)) void  CDAudio_Update() {}

