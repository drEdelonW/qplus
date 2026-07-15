#include "LeafModel.h"
#include "host.h"


mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model) {
    if ((!model) ||
        (!model->nodes)
        )   Host_SysError("Mod_PointInLeaf: bad model");

    mNode_p node = model->nodes;
    while (1) {
#if 0
        if (node->contents < CONTENTS_NODE) // isLeaf
            return (mLeaf_p)node;
        else { // isNode
            float d = DotProduct(p, node->plane->normal) - node->plane->dist;
            if (d > 0.f)    node = node->children[PsFront];
            else            node = node->children[PsBack];
        }
#else
        switch (NodeKind(node)) {
        default: Host_Error("Mod_PointInLeaf"); break;
        case isLeaf: {
            return (mLeaf_p)node;
        } break;
        case isNode: {
            float d = DotProduct(p, node->plane->normal) - node->plane->dist;
            if (d > 0.f)    node = node->children[PsFront];
            else            node = node->children[PsBack];
        } break;
        }
#endif
    }
    return NULL; // never reached
}

