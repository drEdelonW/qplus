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
        if (plane->dist <= emins[plane->type])  return 1;
        if (plane->dist >= emaxs[plane->type])  return 2;
        return 3;
    }
#endif

    // general case
    float dist1, dist2;
    switch (plane->signbits) {
    case 0:
        dist1 =
            plane->normal[0] * emaxs[0] +
            plane->normal[1] * emaxs[1] +
            plane->normal[2] * emaxs[2];
        dist2 =
            plane->normal[0] * emins[0] +
            plane->normal[1] * emins[1] +
            plane->normal[2] * emins[2];
        break;
    case 1:
        dist1 =
            plane->normal[0] * emins[0] +
            plane->normal[1] * emaxs[1] +
            plane->normal[2] * emaxs[2];
        dist2 =
            plane->normal[0] * emaxs[0] +
            plane->normal[1] * emins[1] +
            plane->normal[2] * emins[2];
        break;
    case 2:
        dist1 =
            plane->normal[0] * emaxs[0] +
            plane->normal[1] * emins[1] +
            plane->normal[2] * emaxs[2];
        dist2 =
            plane->normal[0] * emins[0] +
            plane->normal[1] * emaxs[1] +
            plane->normal[2] * emins[2];
        break;
    case 3:
        dist1 =
            plane->normal[0] * emins[0] +
            plane->normal[1] * emins[1] +
            plane->normal[2] * emaxs[2];
        dist2 =
            plane->normal[0] * emaxs[0] +
            plane->normal[1] * emaxs[1] +
            plane->normal[2] * emins[2];
        break;
    case 4:
        dist1 =
            plane->normal[0] * emaxs[0] +
            plane->normal[1] * emaxs[1] +
            plane->normal[2] * emins[2];
        dist2 =
            plane->normal[0] * emins[0] +
            plane->normal[1] * emins[1] +
            plane->normal[2] * emaxs[2];
        break;
    case 5:
        dist1 =
            plane->normal[0] * emins[0] +
            plane->normal[1] * emaxs[1] +
            plane->normal[2] * emins[2];
        dist2 =
            plane->normal[0] * emaxs[0] +
            plane->normal[1] * emins[1] +
            plane->normal[2] * emaxs[2];
        break;
    case 6:
        dist1 =
            plane->normal[0] * emaxs[0] +
            plane->normal[1] * emins[1] +
            plane->normal[2] * emins[2];
        dist2 =
            plane->normal[0] * emins[0] +
            plane->normal[1] * emaxs[1] +
            plane->normal[2] * emaxs[2];
        break;
    case 7:
        dist1 =
            plane->normal[0] * emins[0] +
            plane->normal[1] * emins[1] +
            plane->normal[2] * emins[2];
        dist2 =
            plane->normal[0] * emaxs[0] +
            plane->normal[1] * emaxs[1] +
            plane->normal[2] * emaxs[2];
        break;
    default:
        dist1 = dist2 = 0;  // shut up compiler
        BOPS_Error();
        break;
    }

#if 0
    vec3_t corners[2];
    for (int i = 0; i < VECT_DIM; i++) {
        if (plane->normal[i] < 0) {
            corners[0][i] = emins[i];
            corners[1][i] = emaxs[i];
        }
        else {
            corners[1][i] = emins[i];
            corners[0][i] = emaxs[i];
        }
    }
    dist = DotProduct(plane->normal, corners[0]) - plane->dist;
    dist2 = DotProduct(plane->normal, corners[1]) - plane->dist;
    sides = 0;
    if (dist1 >= 0)     sides = 1;
    if (dist2 < 0)      sides |= 2;

#endif

    int sides = 0;
    if (dist1 >= plane->dist)   sides |= 1;
    if (dist2 < plane->dist)    sides |= 2;

#ifdef PARANOID
    if (sides == 0) Host_SysError("BoxOnPlaneSide: sides==0");
#endif

    return sides;
}

#endif

#include "model.h"
#include "endian_tools.h"

/*
=================
Mod_LoadPlanes
=================
*/

void Mod_LoadPlanes(Lump_p l) {
    dPlane_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))        Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    mPlane_p out = Hunk_AllocName(count * 2 * sizeof(*out), Mod_loadName);

    _loadModel->planes = out;
    _loadModel->numplanes = count;

    for (int i = 0; i < count; i++, in++, out++) {
        int bits = 0;
        for (int j = 0; j < VECT_DIM; j++) {
            out->normal[j] = LittleFloat(in->normal[j]);
            if (out->normal[j] < 0)
                bits |= 1 << j;
        }

        out->dist = LittleFloat(in->dist);
        out->type = LittleLong(in->type);
        out->signbits = bits;
    }
}
