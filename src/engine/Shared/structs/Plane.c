#include "Plane.h"
#include "host.h"
#include "z_hunk.h"

/*
==================
BOPS_Error

Split out like this for ASM to call.
==================
*/
void BOPS_Error() { Host_SysError("BoxOnPlaneSide:  Bad signbits"); }


#if !id386

/*
==================
BoxOnPlaneSide

BoxPlaneSide_t BPsFront, BPsBack, or BPsBoth
==================
*/
// bbox_indexes[signbits][axis] -> bounds selector for dist1 (near corner)
// dist2 selector is always ^ 1
static const int bbox_indexes[8][3] = {
    {1, 1, 1},  // 0b000: all positive
    {0, 1, 1},  // 0b001: x negative
    {1, 0, 1},  // 0b010: y negative
    {0, 0, 1},  // 0b011: x,y negative
    {1, 1, 0},  // 0b100: z negative
    {0, 1, 0},  // 0b101: x,z negative
    {1, 0, 0},  // 0b110: y,z negative
    {0, 0, 0},  // 0b111: all negative
};
BoxPlaneSide_t BoxOnPlaneSide(BBox_t eBB, mPlane_p plane) {
    const int* pIdx = bbox_indexes[plane->signbits];
    float dist1 = DotProduct(plane->normal, VecXYZ(
        eBB.bounds[pIdx[X_AX]  ].x,
        eBB.bounds[pIdx[Y_AX]  ].y,
        eBB.bounds[pIdx[Z_AX]  ].z
    ));
    float dist2 = DotProduct(plane->normal, VecXYZ(  // For dist2 we just invert the selection
        eBB.bounds[pIdx[X_AX]^1].x,
        eBB.bounds[pIdx[Y_AX]^1].y,
        eBB.bounds[pIdx[Z_AX]^1].z
    ));

    BoxPlaneSide_t sides = BPsNone;
    if (dist1 >= plane->dist)   sides |= BPsFront;
    if (dist2 < plane->dist)    sides |= BPsBack;

#ifdef PARANOID
    if (sides == PsNone) Host_SysError("BoxOnPlaneSide: sides==0");
#endif

    return sides;
}

#endif

#include "model.h"
#include "BrushModel.h"
#include "endian_tools.h"


void Mod_LoadPlanes(Lump_p Lump_in) {
    dPlane_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(*in))        Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = Lump_in->fileLen / sizeof(*in);
    mPlane_p out = Hunk_AllocName(TWICE(count) * sizeof(*out), Mod_loadName);

    _loadModel->planes = out;
    _loadModel->numplanes = count;

    for (int i = 0; i < count; i++, in++, out++) {
        int bits = 0x00;
        for (int j = 0; j < VECT_DIM; j++) {
            out->normal.v[j] = LittleFloat(in->normal.v[j]);
            if (out->normal.v[j] < 0.0f)
                bits |= 1 << j;
        }

        out->dist = LittleFloat(in->dist);
        out->type = LittleLong(in->type);
        out->signbits = bits;
    }
}
