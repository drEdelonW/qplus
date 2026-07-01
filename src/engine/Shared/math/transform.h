#pragma once
#include "vector.h"
#include "angle.h"

typedef struct {
    vec3_t forward; // fv
    vec3_t right;   // rv
    vec3_t up;      // uv
} Basis_t;
typedef Basis_t* Basis_p;

extern Basis_t  BS; // it was vpn, vright, vup;
extern Basis_t  base_BS; // it was base_vpn, base_vright, base_vup;

#ifdef __cplusplus
extern "C" {
#endif

    // void    AngleVectors(vec3_t angles, vec3_p forward, vec3_p right, vec3_p up);
    void    AngleToBasis(ang3_t angles, Basis_p bs);
    Basis_t GetBasis(ang3_t angles);

#ifdef __cplusplus
}
#endif