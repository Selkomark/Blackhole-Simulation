#pragma once
#include <cmath>
#include <iostream>

struct Vector3 {
  double x, y, z;

  constexpr Vector3() : x(0), y(0), z(0) {}
  constexpr Vector3(double x, double y, double z) : x(x), y(y), z(z) {}

  Vector3 operator+(const Vector3 &other) const {
    return {x + other.x, y + other.y, z + other.z};
  }
  Vector3 operator-(const Vector3 &other) const {
    return {x - other.x, y - other.y, z - other.z};
  }
  Vector3 operator*(double scalar) const {
    return {x * scalar, y * scalar, z * scalar};
  }
  Vector3 operator/(double scalar) const {
    return {x / scalar, y / scalar, z / scalar};
  }
  Vector3 operator*(const Vector3 &other) const {
    return {x * other.x, y * other.y, z * other.z};
  }
  Vector3 operator/(const Vector3 &other) const {
    return {x / other.x, y / other.y, z / other.z};
  }

  Vector3 &operator+=(const Vector3 &other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }
  Vector3 &operator-=(const Vector3 &other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }
  Vector3 &operator*=(double scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
  }

  double dot(const Vector3 &other) const {
    return x * other.x + y * other.y + z * other.z;
  }
  Vector3 cross(const Vector3 &other) const {
    return {y * other.z - z * other.y, z * other.x - x * other.z,
            x * other.y - y * other.x};
  }

  double lengthSquared() const { return x * x + y * y + z * z; }
  double length() const { return std::sqrt(lengthSquared()); }

  Vector3 normalized() const {
    double len = length();
    return (len > 0) ? *this / len : Vector3{0, 0, 0};
  }
};

inline Vector3 operator*(double scalar, const Vector3 &v) { return v * scalar; }

struct Ray {
  Vector3 origin;
  Vector3 direction;

  Ray(const Vector3 &origin, const Vector3 &direction)
      : origin(origin), direction(direction.normalized()) {}
};
