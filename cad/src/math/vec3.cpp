#include "vec3.h"
#include <cmath>

vec3 vec3::operator/(float rhs) const {
    return vec3(x / rhs, y / rhs, z / rhs);
}

[[nodiscard]] vec3 vec3::normalize() const {
    const float length = sqrtf(x * x + y * y + z * z);
    if (length > 0) {
        return vec3(x, y, z) / length;
    }
    return {0.0f, 0.0f, 0.0f};
}

[[nodiscard]] vec3 vec3::operator-() const {
    return vec3(-x, -y, -z);
}

float dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
