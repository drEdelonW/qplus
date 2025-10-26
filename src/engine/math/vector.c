
#include "vector.h"
#include <math.h>


/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t dst, const vec3_t src) {
    float minelem = 1.0F;

    /*
    ** find the smallest magnitude axially aligned vector
    */
    int pos = 0;
    for (int i = 0; i < VECT_DIM; i++) {
        if (fabs(src[i]) < minelem) {
            pos = i;
            minelem = fabs(src[i]);
        }
    }
    vec3_t tempvec = { 0.0F, 0.0F, 0.0F };
    tempvec[pos] = 1.0F;

    /*
    ** project the point onto the plane defined by src
    */
    ProjectPointOnPlane(dst, tempvec, src);

    /*
    ** normalize the result
    */
    VectorNormalize(dst);
}
vec_t _DotProduct(const vec3_t v1, const vec3_t v2) {
    return
        v1[0] * v2[0] +
        v1[1] * v2[1] +
        v1[2] * v2[2];
}

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal) {
    float inv_denom = 1.0F / _DotProduct(normal, normal);
    float d = _DotProduct(normal, p) * inv_denom;

    vec3_t n;
    n[0] = normal[0] * inv_denom;
    n[1] = normal[1] * inv_denom;
    n[2] = normal[2] * inv_denom;

    dst[0] = p[0] - d * n[0];
    dst[1] = p[1] - d * n[1];
    dst[2] = p[2] - d * n[2];
}



bool VectorCompare(vec3_t v1, vec3_t v2) {
    for (int i = 0; i < VECT_DIM; i++)
        if (v1[i] != v2[i])
            return false;
    return true;
}

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc) {
    vecc[0] = veca[0] + scale * vecb[0];
    vecc[1] = veca[1] + scale * vecb[1];
    vecc[2] = veca[2] + scale * vecb[2];
}

vec_t DotProduct(vec3_t v1, vec3_t v2) {
    return
        v1[0] * v2[0] +
        v1[1] * v2[1] +
        v1[2] * v2[2];
}

void VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out) {
    out[0] = veca[0] - vecb[0];
    out[1] = veca[1] - vecb[1];
    out[2] = veca[2] - vecb[2];
}

void VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out) {
    out[0] = veca[0] + vecb[0];
    out[1] = veca[1] + vecb[1];
    out[2] = veca[2] + vecb[2];
}

void VectorCopy(vec3_t in, vec3_t out) {
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross) {
    cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
    cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
    cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}


vec_t Length(vec3_t v) {
    float length = 0;
    for (int i = 0; i < VECT_DIM; i++)
        length += v[i] * v[i];
    length = sqrt(length);  // FIXME

    return length;
}

float VectorNormalize(vec3_t v) {
    float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    length = sqrt(length);  // FIXME

    if (length) {
        float ilength = 1 / length;
        v[0] *= ilength;
        v[1] *= ilength;
        v[2] *= ilength;
    }

    return length;

}

void VectorInverse(vec3_t v) {
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
}

void VectorScale(vec3_t in, vec_t scale, vec3_t out) {
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
}
