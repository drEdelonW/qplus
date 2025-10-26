#pragma once
#include "vector.h"

#define VEC3_ENABLE_MUL_AS_CROSS 0

typedef float vect_t;

class Vector3D {
public:
    vect_t x, y, z;

    Vector3D(vect_t _x = 0.0f, vect_t _y = 0.0f, vect_t _z = 0.0f)  noexcept : x(_x), y(_y), z(_z) {};

    Vector3D(vec3_t v)  noexcept : x(v[0]), y(v[1]), z(v[2]) {};
    Vector3D& operator=(const vec3_t v) noexcept { x = v[0]; y = v[1]; z = v[2]; return *this; }
    void toVec3(vec3_t out) const noexcept { out[0] = x; out[1] = y; out[2] = z; }

    // vec3_t toVec3() const noexcept {
    //     vec3_t v = { x, y, z };
    //     return v;
    // }
#if 0
#   define LOG_PRINT
    void print() const noexcept;
    void printXML() const noexcept;
#endif

    Vector3D  operator+(const  Vector3D& other) const noexcept;
    Vector3D& operator+=(const Vector3D& other) noexcept;
    Vector3D  operator-(const  Vector3D& other) const noexcept;
    Vector3D& operator-=(const Vector3D& other) noexcept;

#if VEC3_ENABLE_MUL_AS_CROSS    // operator* used as cross product
    inline Vector3D operator*(const Vector3D& o) const noexcept { return cross(o); }
#endif

    Vector3D  operator*(vect_t  scalar) const noexcept;
    Vector3D& operator*=(vect_t scalar) noexcept;
    Vector3D  operator/(vect_t  scalar) const noexcept;
    Vector3D& operator/=(vect_t scalar) noexcept;

    vect_t     dot(const   Vector3D& other) const noexcept;
    Vector3D  cross(const Vector3D& other) const noexcept;

    vect_t length() const noexcept;
    Vector3D normalize() const noexcept;
    bool isZero() const noexcept;

    Vector3D operator-() const noexcept {
        return Vector3D(-x, -y, -z);
    }

    Vector3D toRad() const noexcept;
    Vector3D toDeg() const noexcept;
};
