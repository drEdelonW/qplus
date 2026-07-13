#pragma once

#include "model.h"

#define MAX_LBM_HEIGHT (480)

#ifdef __cplusplus
extern "C" {
#endif

    void Mod_LoadAliasModel(Model_p mod, TypeLess_ptr buffer);  // .mdl file

#ifdef __cplusplus
}
#endif