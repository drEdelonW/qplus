
#include "vector.h"

#include <math.h>
#include "Vector3d.hpp"


/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t dst, const vec3_t src) {
    float minelem = 1.0F;

    // find the smallest magnitude axially aligned vector
    int pos = 0;
    for (int i = 0; i < VECT_DIM; i++)
        if (fabs(src[i]) < minelem) {
            pos = i;
            minelem = fabs(src[i]);
        }

    vec3_t tempvec = { 0.0f, 0.0f, 0.0f };
    tempvec[pos] = 1.0F;

    ProjectPointOnPlane(dst, tempvec, src); // project the point onto the plane defined by src
    VectorNormalize(dst);    // normalize the result
}

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal) {
    float inv_denom = 1.0F / DotProduct(normal, normal);
    float d = DotProduct(normal, p) * inv_denom;

    vec3_t n;
    n[0] = normal[0] * inv_denom;
    n[1] = normal[1] * inv_denom;
    n[2] = normal[2] * inv_denom;

    dst[0] = p[0] - d * n[0];
    dst[1] = p[1] - d * n[1];
    dst[2] = p[2] - d * n[2];
}

bool VectorCompare(vec3_t const v1, vec3_t const v2) {
    for (int i = 0; i < VECT_DIM; i++)
        if (v1[i] != v2[i])
            return false;
    return true;
}

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc) {
#if 1
    Vector3D aV(veca);
    Vector3D bV(vecb);
    (aV + (bV * scale)).toVec3(vecc);
#else
    vecc[0] = veca[0] + scale * vecb[0];
    vecc[1] = veca[1] + scale * vecb[1];
    vecc[2] = veca[2] + scale * vecb[2];
#endif
}

vec_t DotProduct(vec3_t const v1, vec3_t const v2) {
#if 0
    Vector3D aV(v1);
    Vector3D bV(v2);
    return aV.dot(bV);
#else
    return
        (v1[0] * v2[0]) +
        (v1[1] * v2[1]) +
        (v1[2] * v2[2]);
#endif
}

void VectorSubtract(vec3_t const veca, vec3_t const vecb, vec3_t out) {
#if 1
    Vector3D aV(veca);
    Vector3D bV(vecb);
    (aV - bV).toVec3(out);
#else
    out[0] = veca[0] - vecb[0];
    out[1] = veca[1] - vecb[1];
    out[2] = veca[2] - vecb[2];
#endif
}

void VectorAdd(vec3_t const veca, vec3_t const vecb, vec3_t out) {
#if 1
    Vector3D aV(veca);
    Vector3D bV(vecb);
    (aV + bV).toVec3(out);
#else
    out[0] = veca[0] + vecb[0];
    out[1] = veca[1] + vecb[1];
    out[2] = veca[2] + vecb[2];
#endif
}

void VectorCopy(vec3_t const in, vec3_t out) {
#if 1
    Vector3D V(in);
    V.toVec3(out);
#else
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
#endif
}

void CrossProduct(vec3_t const v1, vec3_t const v2, vec3_t cross) {
#if 1
    Vector3D aV(v1);
    Vector3D bV(v2);
    aV.cross(v2).toVec3(cross);
#else
    cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
    cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
    cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
#endif
}


vec_t Length(vec3_t const v) {
#if 1
    Vector3D V(v);
    return V.length();
#else
    float length = 0;
    for (int i = 0; i < VECT_DIM; i++)
        length += v[i] * v[i];
    length = sqrt(length);  // FIXME

    return length;
#endif
}

float VectorNormalize(vec3_t v) {
#if 1
    Vector3D V(v);
    float len = V.length();
    V.normalize().toVec3(v);
    return len;
#else
    float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    length = sqrt(length);  // FIXME

    if (length) {
        float ilength = 1 / length;
        v[0] *= ilength;
        v[1] *= ilength;
        v[2] *= ilength;
    }

    return length;
#endif
}

void VectorInverse(vec3_t v) {
#if 1
    Vector3D V(v);
    (-V).toVec3(v);
#else
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
#endif
}

void VectorScale(vec3_t const in, vec_t scale, vec3_t out) {
#if 1
    Vector3D V(in);
    (V * scale).toVec3(out);
#else
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
#endif
}
