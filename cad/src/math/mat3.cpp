#include "mat3.h"

#include <cassert>
#include <cmath>
#include <stdexcept>

float& mat3::operator[](const int i) {
    return m[i];
}

const float& mat3::operator[](const int i) const {
    return m[i];
}

float& mat3::operator()(const unsigned int row, const unsigned int col) {
    return m[row * 3 + col];
}

const float& mat3::operator()(const unsigned int row, const unsigned int col) const {
    return m[row * 3 + col];
}

[[nodiscard]] bool mat3::all_close(const mat3& other) const {
    for (int i = 0; i < 9; i++) {
        if (abs(m[i] - other[i]) > 1e-5) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] vec3 mat3::operator*(const vec3& rhs) const {
    vec3 result = {};
    result.x = m[0] * rhs.x + m[1] * rhs.y + m[2] * rhs.z;
    result.y = m[3] * rhs.x + m[4] * rhs.y + m[5] * rhs.z;
    result.z = m[6] * rhs.x + m[7] * rhs.y + m[8] * rhs.z;
    return result;
}

[[nodiscard]] mat3 mat3::operator*(const mat3& rhs) const {
    mat3 result = {};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            for (int k = 0; k < 3; ++k) {
                result(row, col) += (*this)(row, k) * rhs(k, col);
            }
        }
    }
    return result;
}

mat3 mat3::eye() {
    mat3 result = {};
    result.m[0] = 1.0f;
    result.m[4] = 1.0f;
    result.m[8] = 1.0f;
    return result;
}

[[nodiscard]] mat3 mat3::t() const {
    mat3 result = {};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result(i, j) = (*this)(j, i);
        }
    }
    return result;
}

[[nodiscard]] float mat3::det() const {
    // Using rule of Sarrus
    // https://en.wikipedia.org/wiki/Rule_of_Sarrus
    return m[0] * (m[4] * m[8] - m[5] * m[7])  // a(ei - fh)
         - m[1] * (m[3] * m[8] - m[5] * m[6])  // b(di - fg)
         + m[2] * (m[3] * m[7] - m[4] * m[6]); // c(dh - eg)
}

[[nodiscard]] float mat3::minor(const int i, const int j) const {
    const int rows[2] = { (i == 0 ? 1 : 0), (i == 2 ? 1 : 2) };
    const int cols[2] = { (j == 0 ? 1 : 0), (j == 2 ? 1 : 2) };

    return m[rows[0] * 3 + cols[0]] * m[rows[1] * 3 + cols[1]] -
           m[rows[0] * 3 + cols[1]] * m[rows[1] * 3 + cols[0]];
}

[[nodiscard]] mat3 mat3::adjugate() const {
    mat3 result = {};

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            const float sign = (i + j) % 2 == 0 ? 1.0f : -1.0f;
            result(i, j) = sign * minor(i, j);
        }
    }

    return result.t();
}

[[nodiscard]] mat3 mat3::inv() const {
    const float determinant = det();

    if (std::abs(determinant) < 1e-5f) {
        throw std::runtime_error("Matrix is singular and cannot be inverted.");
    }

    const mat3 adj = adjugate();

    mat3 result = {};

    for (int i = 0; i < 9; ++i) {
        result[i] = adj[i] / determinant;
    }

    assert((result * (*this)).all_close(mat3::eye()));

    return result;
}