
#include "sound.h"
#include "mem_placement.h"

__weak bool SNDDMA_Init() { fakedma = true; return false; }
__weak int  SNDDMA_GetDMAPos() { return 0; }
__weak void SNDDMA_Shutdown() {}
__weak void SNDDMA_Submit() {}
// __weak void SNDDMA_BeginPainting() {}
// __weak void SNDDMA_EndPainting() {}