// Vector3D.cpp
#include "Vector3d.hpp"
// #include <cmath>
// #include <limits>
#include <math.h>   // sqrtf, fabsf

// #include "terminal_tools.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif


void Vector3D::print() const {
    // LOG("Vector3D(x: %+.3f, y: %+.3f, z: %+.3f)", x, y, z);
}
void Vector3D::printXML() const {
    // LOG("<Vector3D x=\"%f\" y=\"%f\" z=\"%f\" />\n", x, y, z);
}

Vector3D Vector3D::operator+(const Vector3D& other) const {
    return Vector3D((x + other.x), (y + other.y), (z + other.z));
}

Vector3D& Vector3D::operator+=(const Vector3D& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vector3D Vector3D::operator-(const Vector3D& other) const {
    return Vector3D((x - other.x), (y - other.y), (z - other.z));
}

Vector3D& Vector3D::operator-=(const Vector3D& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}


Vector3D Vector3D::operator*(float scalar) const {
    return Vector3D((x * scalar), (y * scalar), (z * scalar));
}

Vector3D& Vector3D::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

Vector3D Vector3D::operator/(float scalar) const {
    if (scalar != 0.0f) {
#if 1
        float i_scalar = 1.0f / scalar;
        return Vector3D((x * i_scalar), (y * i_scalar), (z * i_scalar));
    }
    else
        return Vector3D(0.0f, 0.0f, 0.0f);
#else
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

Vector3D& Vector3D::operator/=(float scalar) {
#if 1
    if (scalar == 0.0f) {
        x = y = z = 0.0f;
        return *this;
    }
    float i_scalar = 1.0f / scalar;
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
float Vector3D::dot(const Vector3D& other) const {
    return (x * other.x) + (y * other.y) + (z * other.z);
}

Vector3D Vector3D::cross(const Vector3D& other) const {
#if 1
    // treat as cross product (no NaN branches)
    return
        Vector3D(
            (y * other.z) - (z * other.y),
            (z * other.x) - (x * other.z),
            (x * other.y) - (y * other.x)
        );
#else
    float nx = (y * other.z) - (z * other.y);
    float ny = (z * other.x) - (x * other.z);
    float nz = (x * other.y) - (y * other.x);

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

float Vector3D::length() const {
    return
#if 1
    sqrtf
#else
    std::sqrt
#endif
    ((x * x) + (y * y) + (z * z));
}

Vector3D Vector3D::normalize() const {
#if 1
    float l2 = (x * x) + (y * y) + (z * z);
    if (l2 > 0.0f) {
        float inv = 1.0f / sqrtf(l2);
        return Vector3D(x * inv, y * inv, z * inv);
    }
    return Vector3D(0.f, 0.f, 0.f);
#else
    float len = length();
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

bool Vector3D::isZero() const {
    return (x == 0.0f) && (y == 0.0f) && (z == 0.0f);
}

Vector3D Vector3D::toRad() const {
    return Vector3D(x, y, z) * (M_PI / 180.0f);
}

Vector3D Vector3D::toDeg() const {
    return Vector3D(x, y, z) * (180.0f / M_PI);
}
