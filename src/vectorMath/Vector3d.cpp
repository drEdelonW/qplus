// Vector3D.cpp
#include "Vector3d.hpp"
// #include <cmath>
// #include <limits>
#include <math.h>   // sqrtf, fabsf


#ifdef LOG_PRINT
#   include "terminal_tools.h"
void Vector3D::print() const noexcept {
    LOG("Vector3D(x: %+.3f, y: %+.3f, z: %+.3f)", x, y, z);
}
void Vector3D::printXML() const noexcept {
    LOG("<Vector3D x=\"%f\" y=\"%f\" z=\"%f\" />\n", x, y, z);
}
#endif

Vector3D Vector3D::operator+(const Vector3D& other) const noexcept {
    return Vector3D((x + other.x), (y + other.y), (z + other.z));
}

Vector3D& Vector3D::operator+=(const Vector3D& other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vector3D Vector3D::operator-(const Vector3D& other) const noexcept {
    return Vector3D((x - other.x), (y - other.y), (z - other.z));
}

Vector3D& Vector3D::operator-=(const Vector3D& other) noexcept {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}


Vector3D Vector3D::operator*(vect_t scalar) const noexcept {
    return Vector3D((x * scalar), (y * scalar), (z * scalar));
}

Vector3D& Vector3D::operator*=(vect_t scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

Vector3D Vector3D::operator/(vect_t scalar) const noexcept {
#if 1
    if (scalar != 0.0f) {
        vect_t i_scalar = 1.0f / scalar;
        return Vector3D((x * i_scalar), (y * i_scalar), (z * i_scalar));
    }
    else
        return Vector3D(0.0f, 0.0f, 0.0f);
#else
    if (scalar != 0.0f) {
        return Vector3D((x / scalar), (y / scalar), (z / scalar));
    }
    else {
        return Vector3D(
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN()
        );
    }
#endif
}

Vector3D& Vector3D::operator/=(vect_t scalar) noexcept {
#if 1
    if (scalar == 0.0f) {
        x = y = z = 0.0f;
        return *this;
    }
    vect_t i_scalar = 1.0f / scalar;
    x *= i_scalar;
    y *= i_scalar;
    z *= i_scalar;
#else
    if (scalar != 0.0f) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
    }
    else {
        x = std::numeric_limits<float>::quiet_NaN();
        y = std::numeric_limits<float>::quiet_NaN();
        z = std::numeric_limits<float>::quiet_NaN();
    }
#endif
    return *this;
}

// Скалярное умножение
vect_t Vector3D::dot(const Vector3D& other) const noexcept {
    return (x * other.x) + (y * other.y) + (z * other.z);
}

Vector3D Vector3D::cross(const Vector3D& other) const noexcept {
#if 1
    // treat as cross product (no NaN branches)
    return
        Vector3D(
            (y * other.z) - (z * other.y),
            (z * other.x) - (x * other.z),
            (x * other.y) - (y * other.x)
        );
#else
    vect_t nx = (y * other.z) - (z * other.y);
    vect_t ny = (z * other.x) - (x * other.z);
    vect_t nz = (x * other.y) - (y * other.x);

    // Проверка на коллинеарность
    if ((nx == 0.0f) && (ny == 0.0f) && (nz == 0.0f)) {
        return Vector3D(
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN()
        );
    }
    return Vector3D(nx, ny, nz);
#endif
}

vect_t Vector3D::length() const noexcept {
    return
#if 1
    sqrtf
#else
    std::sqrt
#endif
    ((x * x) + (y * y) + (z * z));
}

Vector3D Vector3D::normalize() const noexcept {
#if 1
    vect_t l2 = (x * x) + (y * y) + (z * z);
    if (l2 > 0.0f) {
        vect_t inv = 1.0f / sqrtf(l2);
        return Vector3D(x * inv, y * inv, z * inv);
    }
    return Vector3D(0.f, 0.f, 0.f);
#else
    vect_t len = length();
    if (len != 0) {
        return *this / len;
    }
    else {
        return Vector3D(
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN()
        );
    }
#endif
}

bool Vector3D::isZero() const noexcept {
    return (x == 0.0f) && (y == 0.0f) && (z == 0.0f);
}
#if __cplusplus >= 201703L
    // C++17+: inline constexpr avoids .cpp definition
    inline static constexpr float DEG2RAD = 0.01745329251994329577f;
    inline static constexpr float RAD2DEG = 57.2957795130823208768f;
#else
    // Pre-C++17: define in .cpp (see below)
    static const float DEG2RAD;
    static const float RAD2DEG;
#endif

Vector3D Vector3D::toRad() const noexcept {
    return Vector3D(x, y, z) * DEG2RAD;
}

Vector3D Vector3D::toDeg() const noexcept {
    return Vector3D(x, y, z) * RAD2DEG;
}
