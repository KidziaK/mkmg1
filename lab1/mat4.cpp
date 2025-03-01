#include "mat4.h"
#include <cmath>

float& mat4::operator()(const unsigned int row, const unsigned int col) {
    return m[row * 4 + col];
}

const float& mat4::operator()(const unsigned int row, const unsigned int col) const {
    return m[row * 4 + col];
}

float& mat4::operator[](const int i) {
    return m[i];
}

const float& mat4::operator[](const int i) const {
    return m[i];
}

mat4 mat4::eye() {
    mat4 result = {};
    result.m[0] = 1.0f;
    result.m[5] = 1.0f;
    result.m[10] = 1.0f;
    result.m[15] = 1.0f;
    return result;
}

mat4 mat4::diag(const float a, const float b, const float c, const float d) {
    mat4 result = {};
    result.m[0] = a;
    result.m[5] = b;
    result.m[10] = c;
    result.m[15] = d;
    return result;
}

[[nodiscard]] bool mat4::all_close(const mat4& other) const {
    for (int i = 0; i < 16; i++) {
        if (abs(m[i] - other[i]) > 1e-5) {
            return false;
        }
    }
    return true;
}

mat4 mat4::operator*(const mat4& rhs) const {
    mat4 result = {};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            for (int k = 0; k < 4; ++k) {
                result(row, col) += (*this)(row, k) * rhs(k, col);
            }
        }
    }
    return result;
}

mat4 mat4::operator-(const mat4& rhs) const {
    mat4 result = {};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result(row, col) = (*this)(row, col) - rhs(row, col);
        }
    }
    return result;
}

[[nodiscard]] vec4 mat4::operator*(const vec4& rhs) const {
    vec4 result = {};
    result.x = m[0] * rhs.x + m[1] * rhs.y + m[2] * rhs.z + m[3] * rhs.w;
    result.y = m[4] * rhs.x + m[5] * rhs.y + m[6] * rhs.z + m[7] * rhs.w;
    result.z = m[8] * rhs.x + m[9] * rhs.y + m[10] * rhs.z + m[11] * rhs.w;
    result.w = m[12] * rhs.x + m[13] * rhs.y + m[14] * rhs.z + m[15] * rhs.w;
    return result;
}

[[nodiscard]] mat4 mat4::t() const {
    mat4 result = {};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result(i, j) = (*this)(j, i);
        }
    }
    return result;
}

[[nodiscard]] vec3 mat4::translation() const {
    vec3 result = {};
    result.x = (*this)(0, 3);
    result.y = (*this)(1, 3);
    result.z = (*this)(2, 3);
    return result;
}

[[nodiscard]] vec4 mat4::affine_part() const {
    vec4 result = {};
    result.x = (*this)(0, 3);
    result.y = (*this)(1, 3);
    result.z = (*this)(2, 3);
    result.w = (*this)(3, 3);
    return result;
}

[[nodiscard]] mat3 mat4::linear_part() const {
    mat3 result = {};

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            result(row, col) = (*this)(row, col);
        }
    }

    return result;
}

[[nodiscard]] mat4 mat4::inv() const {
    //    A   = [   M            b     ]
    //          [   0            1     ]
    //
    // inv(A) = [ inv(M)   -inv(M) * b ]
    //          [   0            1     ]
    mat3 linear_part = this->linear_part();
    mat3 inverse = linear_part.inv();

    mat4 result = {};

    vec3 affine_part = -(inverse * this->translation());

    result(0, 3) = affine_part.x;
    result(1, 3) = affine_part.y;
    result(2, 3) = affine_part.z;
    result(3, 3) = m[15];

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            result(row, col) = inverse(row, col);
        }
    }

    return result;
}

[[nodiscard]] float mat4::det() const {
    return linear_part().det() * m[15];
}

mat4 mat4::operator+(const mat4 &rhs) const {
    mat4 result = {};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result(row, col) = (*this)(row, col) + rhs(row, col);
        }
    }
    return result;
}

mat4 mat4::operator*(const float rhs) const {
    mat4 result = {};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result(row, col) = rhs * (*this)(row, col);
        }
    }
    return result;
}

mat4 mat4::operator/(const float rhs) const {
    mat4 result = {};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result(row, col) = (*this)(row, col) / rhs;
        }
    }
    return result;
}
