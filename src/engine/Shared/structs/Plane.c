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

Returns 1, 2, or 1 + 2
==================
*/
PlaneSide_t BoxOnPlaneSide(BBox_t eBB, mPlane_p plane) {
    // Extract bits: 0 if bit is set (negative -> use min), 1 if bit is clear (positive -> use max)
    const int bit_x = ((plane->signbits >> 0) & 1) ^ 1;
    const int bit_y = ((plane->signbits >> 1) & 1) ^ 1;
    const int bit_z = ((plane->signbits >> 2) & 1) ^ 1;

    float dist1 =
        plane->normal.x * eBB.bounds[bit_x].x +
        plane->normal.y * eBB.bounds[bit_y].y +
        plane->normal.z * eBB.bounds[bit_z].z;

    float dist2 =   // For dist2 we just invert the selection
        plane->normal.x * eBB.bounds[bit_x ^ 1].x +
        plane->normal.y * eBB.bounds[bit_y ^ 1].y +
        plane->normal.z * eBB.bounds[bit_z ^ 1].z;

    PlaneSide_t sides = PsNone;
    if (dist1 >= plane->dist)   sides |= PsFront;
    if (dist2 < plane->dist)    sides |= PsBack;

#ifdef PARANOID
    if (sides == PsNone) Host_SysError("BoxOnPlaneSide: sides==0");
#endif

    return sides;
}

#endif

#include "model.h"
#include "BrushModel.h"
#include "endian_tools.h"

/*
=================
Mod_LoadPlanes
=================
*/

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
