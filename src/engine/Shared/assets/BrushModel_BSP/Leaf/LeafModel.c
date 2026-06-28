#include "LeafModel.h"
#include "host.h"
#include "bspfile.h"

/*
===============
Mod_PointInLeaf
===============
*/
mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model) {
    if ((!model) || (!model->nodes))    Host_SysError("Mod_PointInLeaf: bad model");

    mNode_p node = model->nodes;
    while (1) {
        if (node->contents < CONTENTS_NODE)     return (mLeaf_p)node;

        float d = DotProduct(p, node->plane->normal) - node->plane->dist;
        if (d > 0.0f)   node = node->children[0];
        else            node = node->children[1];
    }

    return NULL; // never reached
}

static uint8_t _modNoVis[MAX_MAP_LEAFS / 8];
static uint8_t _decompressed[MAX_MAP_LEAFS / 8];

#include <string.h>
void Mod_LeafClear() {
    memset(_modNoVis, 0xFF, sizeof(_modNoVis));
}

/*
===================
Mod_DecompressVis
===================
*/
uint8_p Mod_DecompressVis(uint8_p in, Model_p model) {
    int row = EIGHTH(model->numleafs + 7);
    uint8_p out = _decompressed;

#if 0
    memcpy(out, in, row);
#else
    if (!in) { // no vis info, so make all visible
        while (row) {
            *out++ = 0xFF;
            row--;
        }
        return _decompressed;
    }

    do {
        if (*in) {
            *out++ = *in++;
            continue;
        }

        int c = in[1];
        in += 2;
        while (c) {
            *out++ = 0x00;
            c--;
        }
    } while ((out - _decompressed) < row);
#endif

    return _decompressed;
}

uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model) {
    return (leaf == model->leafs) ?
        _modNoVis : Mod_DecompressVis(leaf->compressed_vis, model);
}