#pragma once

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

    mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model);
    uint8_p Mod_DecompressVis(uint8_p in, Model_p model);
    uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model);

#ifdef __cplusplus
}
#endif