#pragma once
#include <stdbool.h>

typedef float vec_t;
typedef enum axis_e {
    X_AX = 0,
    Y_AX = 1,
    Z_AX = 2,
    VECT_DIM = 3,
} axis_e;

typedef vec_t vec3_t[VECT_DIM];
typedef vec3_t* vec3_p;

typedef vec_t vec5_t[5];
typedef vec5_t* vec5_p;

#if 1
#define DotProduct(x, y)    \
    (                       \
        x[0] * y[0] +       \
        x[1] * y[1] +       \
        x[2] * y[2]         \
    )
#else
vec_t  DotProduct(vec3_t v1, vec3_t v2);
#endif

#if 1
#define VectorSubtract(a, b, c) \
    {                           \
        c[0] = a[0] - b[0];     \
        c[1] = a[1] - b[1];     \
        c[2] = a[2] - b[2];     \
    }
#else
void   VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);
#endif

#if 1
#define VectorAdd(a, b, c)  \
    {                       \
        c[0] = a[0] + b[0]; \
        c[1] = a[1] + b[1]; \
        c[2] = a[2] + b[2]; \
    }
#else
void   VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);
#endif

#if 1
#define VectorCopy(a, b)    \
    {                       \
        b[0] = a[0];        \
        b[1] = a[1];        \
        b[2] = a[2];        \
    }
#else
void   VectorCopy(vec3_t in, vec3_t out);
#endif

void    VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

bool    VectorCompare(vec3_t v1, vec3_t v2);
vec_t   Length(vec3_t v);

void    CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross);
void    PerpendicularVector(vec3_t dst, const vec3_t src);
void    ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);

float   VectorNormalize(vec3_t v);  // returns vector length
void    VectorInverse(vec3_t v);
void    VectorScale(vec3_t in, vec_t scale, vec3_t out);