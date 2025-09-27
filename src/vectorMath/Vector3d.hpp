#pragma once

class Vector3D {
public:
    float x, y, z;

    Vector3D(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f);

    void print() const;
    void printXML() const;

    Vector3D  operator+(const  Vector3D& other) const;
    Vector3D& operator+=(const Vector3D& other);
    Vector3D  operator-(const  Vector3D& other) const;
    Vector3D& operator-=(const Vector3D& other);

    Vector3D  operator*(float  scalar) const;
    Vector3D& operator*=(float scalar);
    Vector3D  operator*(const  Vector3D& other) const;
    Vector3D  operator/(float  scalar) const;
    Vector3D& operator/=(float scalar);

    float     dot(const   Vector3D& other) const;
    Vector3D  cross(const Vector3D& other) const;

    float length() const;
    Vector3D normalize() const;
    bool isZero() const;

    Vector3D operator-() const {
        return Vector3D(-x, -y, -z);
    }

    Vector3D toRad() const;
    Vector3D toDeg() const;
};
