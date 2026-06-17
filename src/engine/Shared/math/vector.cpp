
#include "vector.h"

#include <math.h>
#include "Vector3d.hpp"


/*
** assumes "src" is normalized
*/
vec3_t/*void*/  PerpendicularVector(/* vec3_p dst, */ const vec3_t src) {
    vec3_t out;
    float minelem = 1.0f;

    // find the smallest magnitude axially aligned vector
    int pos = 0;
    for (int i = 0; i < VECT_DIM; i++)
        if (fabs(src.v[i]) < minelem) {
            pos = i;
            minelem = fabs(src.v[i]);
        }

    vec3_t tempvec = { .x = 0.0f, .y = 0.0f, .z = 0.0f };
    tempvec.v[pos] = 1.0f;

    out = ProjectPointOnPlane(tempvec, src); // project the point onto the plane defined by src
    VectorNormalize(&out);    // normalize the result
    return out;
}

vec3_t/*void*/  ProjectPointOnPlane(/* vec3_p dst, */ const vec3_t p, const vec3_t normal) {
    vec3_t out;
    float inv_denom = 1.0F / DotProduct(normal, normal);
    float d = DotProduct(normal, p) * inv_denom;

    vec3_t n = {
        .x = normal.x * inv_denom,
        .y = normal.y * inv_denom,
        .z = normal.z * inv_denom
    };

    out.x = p.x - d * n.x;
    out.y = p.y - d * n.y;
    out.z = p.z - d * n.z;
    return out;
}

bool VectorCompare(vec3_t const v1, vec3_t const v2) {
    for (int i = 0; i < VECT_DIM; i++)
        if (v1.v[i] != v2.v[i])
            return false;
    return true;
}

vec3_t/*void*/ VectorMA(vec3_t veca, float scale, vec3_t vecb /*, vec3_p vecc */ ) {
    vec3_t out;
#if 1
    Vector3D aV(veca);
    Vector3D bV(vecb);
    (aV + (bV * scale)).toVec3(&out);
#else
    out.x = veca.x + scale * vecb.x;
    out.y = veca.y + scale * vecb.y;
    out.z = veca.z + scale * vecb.z;
#endif
    return out;
}

vec_t DotProduct(vec3_t const v1, vec3_t const v2) {
#if 0
    Vector3D aV(v1);
    Vector3D bV(v2);
    return aV.dot(bV);
#else
    return
        (v1.x * v2.x) +
        (v1.y * v2.y) +
        (v1.z * v2.z);
#endif
}

vec3_t/*void*/ VectorSubtract(vec3_t const veca, vec3_t const vecb /*, vec3_p out */ ) {
    vec3_t out;
#if 1
    Vector3D aV(veca);
    Vector3D bV(vecb);
    (aV - bV).toVec3(&out);
#else
    out.x = veca.x - vecb.x;
    out.y = veca.y - vecb.y;
    out.z = veca.z - vecb.z;
#endif
    return out;
}

vec3_t/*void*/ VectorAdd(vec3_t const veca, vec3_t const vecb /*, vec3_p out */ ) {
    vec3_t out;
#if 1
    Vector3D aV(veca);
    Vector3D bV(vecb);
    (aV + bV).toVec3(&out);
#else
    out.x = veca.x + vecb.x;
    out.y = veca.y + vecb.y;
    out.z = veca.z + vecb.z;
#endif
    return out;
}

void VectorCopy(vec3_t const in, vec3_p out) {
#if 1
    Vector3D V(in);
    V.toVec3(out);
#else
    out->x = in.x;
    out->y = in.y;
    out->z = in.z;
#endif
}

vec3_t/*void*/  CrossProduct(vec3_t const v1, vec3_t const v2 /*, vec3_p cross */ ) {
    vec3_t out;
#if 1
    Vector3D aV(v1);
    Vector3D bV(v2);
    aV.cross(v2).toVec3(&out);
#else
    out.x = v1.y * v2.z - v1.z * v2.y;
    out.y = v1.z * v2.x - v1.x * v2.z;
    out.z = v1.x * v2.y - v1.y * v2.x;
#endif
    return out;
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

float VectorNormalize(vec3_p v) {
#if 1
    Vector3D V(*v);
    float len = V.length();
    V.normalize().toVec3(v);
    return len;
#else
    float length = (x * x) + (y * y) + (z * z);
    length = sqrt(length);  // FIXME

    if (length) {
        float ilength = 1 / length;
        x *= ilength;
        y *= ilength;
        z *= ilength;
    }

    return length;
#endif
}

void VectorInverse(vec3_p v) {
#if 1
    Vector3D V(*v);
    (-V).toVec3(v);
#else
    x = -x;
    y = -y;
    z = -z;
#endif
}

vec3_t/*void*/  VectorScale(vec3_t const in, vec_t const scale/*, vec3_p out */ ) {
    vec3_t out;
#if 1
    Vector3D V(in);
    (V * scale).toVec3(&out);
#else
    out.x = in.x * scale;
    out.y = in.y * scale;
    out.z = in.z * scale;
#endif
    return out;
}

vec3_t VectorAddVal(vec3_t v, vec_t val) {
    v.x += val;
    v.y += val;
    v.z += val;
    return v;
}