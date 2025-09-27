// Vector3D.cpp
#include "Vector3d.hpp"
#include <cmath>
#include <limits>
// #include "terminal_tools.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846f
#endif

Vector3D::Vector3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

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

Vector3D Vector3D::operator*(const Vector3D& other) const {
    float nx = (y * other.z) - (z * other.y);
    float ny = (z * other.x) - (x * other.z);
    float nz = (x * other.y) - (y * other.x);

    // Проверка на коллинеарность
    if ((nx == 0) && (ny == 0) && (nz == 0)) {
        return Vector3D(
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN()
        );
    }

    return Vector3D(nx, ny, nz);
}

Vector3D Vector3D::operator/(float scalar) const {
    if (scalar != 0) {
        return Vector3D((x / scalar), (y / scalar), (z / scalar));
    } else {
        return Vector3D(
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN()
        );
    }
}

Vector3D& Vector3D::operator/=(float scalar) {
    if (scalar != 0) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
    } else {
        x = std::numeric_limits<float>::quiet_NaN();
        y = std::numeric_limits<float>::quiet_NaN();
        z = std::numeric_limits<float>::quiet_NaN();
    }
    return *this;
}

// Скалярное умножение
float Vector3D::dot(const Vector3D& other) const {
    return (x * other.x) + (y * other.y) + (z * other.z);
}

Vector3D Vector3D::cross(const Vector3D& other) const {
    return Vector3D(
        (y * other.z) - (z * other.y),
        (z * other.x) - (x * other.z),
        (x * other.y) - (y * other.x)
    );
}

float Vector3D::length() const {
    return std::sqrt((x * x) + (y * y) + (z * z));
}

Vector3D Vector3D::normalize() const {
    float len = length();
    if (len != 0) {
        return *this / len;
    } else {
        return Vector3D(
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::quiet_NaN()
        );
    }
}

bool Vector3D::isZero() const {
    return x == 0.0f && y == 0.0f && z == 0.0f;
}

Vector3D Vector3D::toRad() const {
    return Vector3D(x, y, z) * (M_PI / 180.0f);
}

Vector3D Vector3D::toDeg() const {
    return Vector3D(x, y, z) * (180.0f / M_PI);
}
