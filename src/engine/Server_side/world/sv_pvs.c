#include "types.h"
#include "world.h"
#include "model.h"
#include "LeafModel.h"
#include "server.h"
#include "q_tools.h"

#define MAX_MAP_LEAFS   (8192) /* 8k */

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
        if (node->contents < CONTENTS_NODE) { // is Leaf?
            if (node->contents != CONTENTS_SOLID) {
                uint8_p pvs = Mod_LeafPVS((mLeaf_p)node, sv.worldmodel);
                for (int i = 0; i < _fatBytes; i++)
                    _fatPvs[i] |= pvs[i];
            }
            return;
        }
        else {  // (node->contents => CONTENTS_NODE) is Node!
            mPlane_p plane = node->plane;
            float d = DotProduct(org, plane->normal) - plane->dist;
            /**/ if (d > 8)     node = node->children[PsFront];
            else if (d < -8)    node = node->children[PsBack];
            else {  // go down both
                SV_AddToFatPVS(org, node->children[PsFront]);     /* Recursive call */
                node = node->children[PsBack];
            }
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
    _fatBytes = DIV8(sv.worldmodel->numleafs + 31);
    Q_memset(_fatPvs, 0, _fatBytes);
    SV_AddToFatPVS(org, sv.worldmodel->nodes);
    return _fatPvs;
}


static uint8_t _decompressed[MAX_MAP_LEAFS / 8];
uint8_p Mod_DecompressVis(uint8_p in, Model_p model) {
    int row = DIV8(model->numleafs + 7);
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


#include <string.h>
static uint8_t _modNoVis[MAX_MAP_LEAFS / 8];
void Mod_LeafClear() {
    memset(_modNoVis, 0xFF, sizeof(_modNoVis));
}

uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model) {
    return (leaf == model->leafs) ?
        _modNoVis : Mod_DecompressVis(leaf->compressed_vis, model);
}

#include "GameRule.h"   // GetSvMaxClients()
static uint8_t _checkPvs[MAX_MAP_LEAFS / 8];
EdIdx PF_newcheckclient(int check) {
    ClampInRange(EdictPlayer1, &check, GetSvMaxClients()); // cycle to the next one
    EdIdx idx = (check == GetSvMaxClients()) ?
        EdictWorld : (EdictPlayer1 + check);
    for (;; idx++) {
        if (idx == EdictPlayer1 + GetSvMaxClients())
            idx = EdictPlayer1;
        if (idx == check) break; // didn't find anything else

        edict_p ent = ED_GetEDictByIdx(idx);
        if (!(ent->inUse) ||
            (ent->v.health <= 0.f) ||
            (((int)ent->v.flags) & FL_NOTARGET)
            )   continue;

        break;  // anything that is a client, or has a client as an enemy
    }

    // get the PVS for the entity
    edict_p ent = ED_GetEDictByIdx(idx);
    mLeaf_p leaf = Mod_PointInLeaf(VectorAdd(ent->v.origin, ent->v.view_ofs), sv.worldmodel);
    memcpy(
        _checkPvs,
        Mod_LeafPVS(leaf, sv.worldmodel),
        DIV8(sv.worldmodel->numleafs + 7)
    );

    return idx;
}



/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient()
=================
*/
#include "GlobVars.h" // RETURN_EDICT
// #define MAX_CHECK 16
// int c_invis, c_notvis;
void PF_checkclient() {
    if ((SV_GetTime() - sv.lastchecktime) >= 0.1) {     // find a new check if on a new frame
        sv.lastcheck = PF_newcheckclient(sv.lastcheck);
        sv.lastchecktime = SV_GetTime();
    }

    // return check if it might be visible
    edict_p ent = ED_GetEDictByIdx(sv.lastcheck);
    if ((!ent->inUse) ||
        (ent->v.health <= 0.f)
        ) {
        RETURN_EDICT(ED_GetEDictByIdx(EdictWorld));   return;
    }

    // if current entity can't possibly see the check entity, return 0
    edict_p self = ED_GetEDictByOffs(pGame()->self);
    mLeaf_p leaf = Mod_PointInLeaf(VectorAdd(self->v.origin, self->v.view_ofs), sv.worldmodel);
    int Leaf = (leaf - sv.worldmodel->leafs) - 1;
    if ((Leaf < 0) ||
        !(_checkPvs[DIV8(Leaf)] & (1 << (Leaf & 7)))
        ) { // c_notvis++;
        RETURN_EDICT(ED_GetEDictByIdx(EdictWorld));   return;
    }
    // c_invis++;  // might be able to see it
    RETURN_EDICT(ent);   return;
}

