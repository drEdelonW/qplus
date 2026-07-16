#pragma once

#define MAX_LBM_HEIGHT (480)
#define MAXALIASVERTS  (2000) // TODO: tune this

#include "model.h"
#ifdef __cplusplus
extern "C" {
#endif

    void Mod_LoadAliasModel(Model_p mod, TypeLess_ptr buffer);  // .mdl file

#ifdef __cplusplus
}
#endif