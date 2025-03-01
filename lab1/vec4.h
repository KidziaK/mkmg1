#pragma once

struct vec4 {
    float x, y, z, w;
    vec4 operator/(float rhs) const;
    [[nodiscard]] vec4 normalize() const;
    [[nodiscard]] vec4 operator-() const;
    [[nodiscard]] vec4 operator+(const vec4& rhs) const;
    [[nodiscard]] vec4 operator*(float rhs) const;
    friend vec4 operator*(float lhs, const vec4& rhs);
    float length() const;
};

float dot(const vec4& a, const vec4& b);;
