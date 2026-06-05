#include "LeafModel.h"
#include "host.h"

/*
===============
Mod_PointInLeaf
===============
*/
mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model) {
    if ((!model) || (!model->nodes))    Host_SysError("Mod_PointInLeaf: bad model");

    mNode_p node = model->nodes;
    while (1) {
        if (node->contents < 0)     return (mLeaf_p)node;

        mPlane_p plane = node->plane;
        float d = DotProduct(p, plane->normal) - plane->dist;
        if (d > 0)  node = node->children[0];
        else        node = node->children[1];
    }

    return NULL; // never reached
}

uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model) {
    if (leaf == model->leafs)       return _modNoVis;
    return Mod_DecompressVis(leaf->compressed_vis, model);
}