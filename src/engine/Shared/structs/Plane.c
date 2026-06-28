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
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, mPlane_p plane) {
#if 0 // this is done by the BOX_ON_PLANE_SIDE macro before calling this
    // function
// fast axial cases
    if (plane->type < 3) {
        if (plane->dist <= emins.v[plane->type])  return 1;
        if (plane->dist >= emaxs.v[plane->type])  return 2;
        return 3;
    }
#endif

#if 0   // it was disabled
    vec3_t corners[2];
    for (int i = 0; i < VECT_DIM; i++) {
        if (plane->normal[i] < 0) {
            corners[0].v[i] = emins.v[i];
            corners[1].v[i] = emaxs.v[i];
        }
        else {
            corners[1].v[i] = emins.v[i];
            corners[0].v[i] = emaxs.v[i];
        }
    }
    dist = DotProduct(plane->normal, corners[0]) - plane->dist;
    dist2 = DotProduct(plane->normal, corners[1]) - plane->dist;
    sides = 0;
    if (dist1 >= 0)     sides |= 1;
    if (dist2 < 0)      sides |= 2;
#else
    // bounds[0] = emins, bounds[1] = emaxs
    const vec3_t bounds[2] = {
        emins,
        emaxs
    };

    // Extract bits: 0 if bit is set (negative -> use min), 1 if bit is clear (positive -> use max)
    const int bit_x = ((plane->signbits >> 0) & 1) ^ 1;
    const int bit_y = ((plane->signbits >> 1) & 1) ^ 1;
    const int bit_z = ((plane->signbits >> 2) & 1) ^ 1;

    float dist1 =
        plane->normal.x * bounds[bit_x].x +
        plane->normal.y * bounds[bit_y].y +
        plane->normal.z * bounds[bit_z].z;

    float dist2 =   // For dist2 we just invert the selection
        plane->normal.x * bounds[bit_x ^ 1].x +
        plane->normal.y * bounds[bit_y ^ 1].y +
        plane->normal.z * bounds[bit_z ^ 1].z;

    int sides = 0;
    if (dist1 >= plane->dist)   sides |= 1;
    if (dist2 < plane->dist)    sides |= 2;
#endif
#ifdef PARANOID
    if (sides == 0) Host_SysError("BoxOnPlaneSide: sides==0");
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
    dPlane_p in = getMapLumpPtr(Lump_in);
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
