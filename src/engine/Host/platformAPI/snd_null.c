
#include "sound.h"
#include "mem_placement.h"

WEAK bool SNDDMA_Init() { fakedma = true; return false; }
WEAK int  SNDDMA_GetDMAPos() { return 0; }
WEAK void SNDDMA_Shutdown() {}
WEAK void SNDDMA_Submit() {}
// WEAK void SNDDMA_BeginPainting() {}
// WEAK void SNDDMA_EndPainting() {}