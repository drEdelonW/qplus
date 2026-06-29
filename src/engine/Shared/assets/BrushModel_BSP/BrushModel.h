#pragma once

#include "model.h"


#define BSPVERSION      (29)
#define IDBRUSHHEADER   (uint32_t)(0x0000001D)

#ifdef __cplusplus
extern "C" {
#endif

    void Mod_LoadBrushModel(Model_p mod, TypeLess_ptr buffer);  // .bsp file

#ifdef __cplusplus
}
#endif
