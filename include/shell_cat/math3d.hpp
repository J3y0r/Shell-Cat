#pragma once
#include <cmath>

namespace shell_cat {

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const {
        float len = length();
        if (len < 1e-6f) return Vec3(0, 0, 0);
        return (*this) * (1.0f / len);
    }
};

inline Vec3 rotate_x(const Vec3& v, float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    return Vec3(v.x, v.y * c - v.z * s, v.y * s + v.z * c);
}

} // namespace shell_cat
