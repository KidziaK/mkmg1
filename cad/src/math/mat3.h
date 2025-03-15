#pragma once

#include "vec3.h"

struct mat3 {
    float m[9];

    static mat3 eye();

    float& operator[](int i);
    const float& operator[](int i) const;
    float& operator()(unsigned int row, unsigned int col);
    const float& operator()(unsigned int row, unsigned int col) const;
    [[nodiscard]] bool all_close(const mat3& other) const;
    [[nodiscard]] vec3 operator*(const vec3& rhs) const;
    [[nodiscard]] mat3 operator*(const mat3& rhs) const;
    [[nodiscard]] mat3 t() const;
    [[nodiscard]] float det() const;
    [[nodiscard]] float minor(int i, int j) const;
    [[nodiscard]] mat3 adjugate() const;
    [[nodiscard]] mat3 inv() const;
};