
#include "sound.h"

__attribute__((weak)) bool SNDDMA_Init() { fakedma = true; return false; }
__attribute__((weak)) int  SNDDMA_GetDMAPos() { return 0; }
__attribute__((weak)) void SNDDMA_Shutdown() {}
__attribute__((weak)) void SNDDMA_Submit() {}
// __attribute__((weak)) void SNDDMA_BeginPainting() {}
// __attribute__((weak)) void SNDDMA_EndPainting() {}