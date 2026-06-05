#include "types.h"
#include "world.h"
#include "model.h"
#include "LeafModel.h"
#include "server.h"
#include "q_tools.h"


/*
    =============================================================================

    The PVS must include a small area around the client to allow head bobbing
    or other small motion on the client side.  Otherwise, a bob might cause an
    entity that should be visible to not show up, especially when the bob
    crosses a waterline.

    =============================================================================
*/

static uint32_t _fatBytes;
static uint8_t  _fatPvs[MAX_MAP_LEAFS / 8];

void SV_AddToFatPVS(vec3_t org, mNode_p node) {
    while (1) {
        // if this is a leaf, accumulate the pvs bits
        if (node->contents < 0) {
            if (node->contents != CONTENTS_SOLID) {
                uint8_p pvs = Mod_LeafPVS((mLeaf_p)node, sv.worldmodel);
                for (int i = 0; i < _fatBytes; i++) {
                    _fatPvs[i] |= pvs[i];
                }
            }
            return;
        }

        mPlane_p plane = node->plane;
        float d = DotProduct(org, plane->normal) - plane->dist;
        if (d > 8)          node = node->children[0];
        else if (d < -8)    node = node->children[1];
        else {  // go down both
            SV_AddToFatPVS(org, node->children[0]);
            node = node->children[1];
        }
    }
}

/*
    =============
    SV_FatPVS

    Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
    given point.
    =============
*/
uint8_p SV_FatPVS(vec3_t org) {
    _fatBytes = (sv.worldmodel->numleafs + 31) >> 3;
    Q_memset(_fatPvs, 0, _fatBytes);
    SV_AddToFatPVS(org, sv.worldmodel->nodes);
    return _fatPvs;
}
