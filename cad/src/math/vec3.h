#pragma once

struct vec3 {
    float x, y, z;
    vec3 operator/(float rhs) const;
    [[nodiscard]] vec3 normalize() const;
    [[nodiscard]] vec3 operator-() const;
};

float dot(const vec3& a, const vec3& b);
