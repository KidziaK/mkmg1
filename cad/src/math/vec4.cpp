#include "vec4.h"
#include <cmath>

vec4 vec4::operator/(const float rhs) const {
    return vec4(x / rhs, y / rhs, z / rhs, w / rhs);
}

[[nodiscard]] vec4 vec4::normalize() const {
    const float length = sqrtf(x * x + y * y + z * z + w * w);
    if (length > 0) {
        return vec4(x, y, z, w) / length;
    }
    return {0.0f, 0.0f, 0.0f, 0.0f};
}

[[nodiscard]] vec4 vec4::operator-() const {
    return vec4(-x, -y, -z, -w);
}

vec4 vec4::operator+(const vec4 &rhs) const {
    return vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

vec4 vec4::operator*(const float rhs) const {
    return vec4(x * rhs, y * rhs, z * rhs, w * rhs);
}

float vec4::length() const {
    return sqrtf(x * x + y * y + z * z + w * w);
}

float dot(const vec4& a, const vec4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

vec4 operator*(float lhs, const vec4& rhs) {
    return vec4{lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w};
}
