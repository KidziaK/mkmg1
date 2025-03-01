#pragma once

struct vec4;
#include "vec4.h"
#include "vec3.h"
#include "mat3.h"
#include "mat4.h"

struct mat4 {
    float m[16];

    static mat4 eye();
    static mat4 diag(float a, float b, float c, float d);

    float& operator()(unsigned int row, unsigned int col);
    const float& operator()(unsigned int row, unsigned int col) const;
    float& operator[](int i);
    const float& operator[](int i) const;
    [[nodiscard]] bool all_close(const mat4& other) const;
    mat4 operator*(const mat4& rhs) const;
    mat4 operator-(const mat4& rhs) const;
    [[nodiscard]] vec4 operator*(const vec4& rhs) const;
    [[nodiscard]] mat4 t() const;
    [[nodiscard]] vec3 translation() const;
    [[nodiscard]] vec4 affine_part() const;
    [[nodiscard]] mat3 linear_part() const;
    [[nodiscard]] mat4 inv() const;
    [[nodiscard]] float det() const;
    mat4 operator+(const mat4& rhs) const;
    mat4 operator*(float rhs) const;
    mat4 operator/(float rhs) const;
};