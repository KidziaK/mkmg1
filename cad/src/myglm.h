#pragma once

#include <cmath>
#include <iostream>

namespace glm {
    struct u16vec2 {
        unsigned short x;
        unsigned short y;
    };

    struct u16vec3 {
        unsigned short x;
        unsigned short y;
        unsigned short z;
    };

    struct vec3 {
        float x, y, z;

        constexpr vec3() : x(0.0f), y(0.0f), z(0.0f) {}
        constexpr vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
        constexpr vec3(const float* arr) : x(arr[0]), y(arr[1]), z(arr[2]) {}

        vec3 operator+(const vec3& other) const {
            return vec3(x + other.x, y + other.y, z + other.z);
        }

        vec3 operator-(const vec3& other) const {
            return vec3(x - other.x, y - other.y, z - other.z);
        }

        vec3 operator*(float scalar) const {
            return vec3(x * scalar, y * scalar, z * scalar);
        }

        vec3 operator/(float scalar) const {
            return vec3(x / scalar, y / scalar, z / scalar);
        }

        float length() const {
            return std::sqrt(x * x + y * y + z * z);
        }

        constexpr vec3& operator=(const vec3& other) {
            x = other.x;
            y = other.y;
            z = other.z;
            return *this;
        }

        vec3 operator-() const {
            return vec3(-x, -y, -z);
        }
    };

    struct mat4 {
        float elements[4][4]; // Column-major

        mat4() {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    elements[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }
        }

        mat4(float diagonal) {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    elements[i][j] = (i == j) ? diagonal : 0.0f;
                }
            }
        }

        mat4 operator*(const mat4& other) const {
            mat4 result;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    result.elements[i][j] = 0.0f;
                    for (int k = 0; k < 4; ++k) {
                        result.elements[i][j] += elements[i][k] * other.elements[k][j];
                    }
                }
            }
            return result;
        }

        vec3 operator*(const vec3& other) const {
            vec3 result;
            result.x = elements[0][0] * other.x + elements[1][0] * other.y + elements[2][0] * other.z + elements[3][0];
            result.y = elements[0][1] * other.x + elements[1][1] * other.y + elements[2][1] * other.z + elements[3][1];
            result.z = elements[0][2] * other.x + elements[1][2] * other.y + elements[2][2] * other.z + elements[3][2];
            float w = elements[0][3] * other.x + elements[1][3] * other.y + elements[2][3] * other.z + elements[3][3];

            if (w != 0.0f && w != 1.0f) {
                result = result / w;
            }
            return result;
        }

        float* operator[](int index) {
            return elements[index];
        }

        const float* operator[](int index) const {
            return elements[index];
        }
    };

    struct quat {
        float w, x, y, z;

        quat() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
        quat(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_) {}

        quat(const mat4& m) {
            float trace = m.elements[0][0] + m.elements[1][1] + m.elements[2][2];
            if (trace > 0.0f) {
                float s = 0.5f / std::sqrt(trace + 1.0f);
                w = 0.25f / s;
                x = (m.elements[2][1] - m.elements[1][2]) * s;
                y = (m.elements[0][2] - m.elements[2][0]) * s;
                z = (m.elements[1][0] - m.elements[0][1]) * s;
            } else {
                int i = 0;
                if (m.elements[1][1] > m.elements[0][0]) i = 1;
                if (m.elements[2][2] > m.elements[i][i]) i = 2;
                static const int next[3] = {1, 2, 0};
                int j = next[i];
                int k = next[j];
                float s = std::sqrt((m.elements[i][i] - (m.elements[j][j] + m.elements[k][k])) + 1.0f);
                float q[4];
                q[i] = s * 0.5f;
                s = 0.5f / s;
                q[3] = (m.elements[k][j] - m.elements[j][k]) * s;
                q[j] = (m.elements[j][i] + m.elements[i][j]) * s;
                q[k] = (m.elements[k][i] + m.elements[i][k]) * s;
                x = q[0];
                y = q[1];
                z = q[2];
                w = q[3];
            }
        }
    };

    mat4 translate(const mat4& m, const vec3& v) {
        mat4 translationMatrix(1.0f);

        translationMatrix.elements[3][0] = v.x;
        translationMatrix.elements[3][1] = v.y;
        translationMatrix.elements[3][2] = v.z;

        return translationMatrix * m;
    }

    mat4 scale(const mat4& m, const vec3& v) {
        mat4 scaleMatrix(1.0f);

        scaleMatrix.elements[0][0] = v.x;
        scaleMatrix.elements[1][1] = v.y;
        scaleMatrix.elements[2][2] = v.z;

        return scaleMatrix * m;
    }

    mat4 rotate(const mat4& m, float angle, const vec3& axis) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        float one_minus_c = 1.0f - c;

        vec3 normalized_axis = axis;
        float length = normalized_axis.length();
        if (length > 0.0f) {
            normalized_axis = normalized_axis / length;
        } else {
            return m;
        }

        mat4 rotation;
        rotation.elements[0][0] = normalized_axis.x * normalized_axis.x * one_minus_c + c;
        rotation.elements[1][0] = normalized_axis.x * normalized_axis.y * one_minus_c - normalized_axis.z * s;
        rotation.elements[2][0] = normalized_axis.x * normalized_axis.z * one_minus_c + normalized_axis.y * s;

        rotation.elements[0][1] = normalized_axis.y * normalized_axis.x * one_minus_c + normalized_axis.z * s;
        rotation.elements[1][1] = normalized_axis.y * normalized_axis.y * one_minus_c + c;
        rotation.elements[2][1] = normalized_axis.y * normalized_axis.z * one_minus_c - normalized_axis.x * s;

        rotation.elements[0][2] = normalized_axis.z * normalized_axis.x * one_minus_c - normalized_axis.y * s;
        rotation.elements[1][2] = normalized_axis.z * normalized_axis.y * one_minus_c + normalized_axis.x * s;
        rotation.elements[2][2] = normalized_axis.z * normalized_axis.z * one_minus_c + c;

        rotation.elements[3][0] = 0.0f;
        rotation.elements[3][1] = 0.0f;
        rotation.elements[3][2] = 0.0f;
        rotation.elements[0][3] = 0.0f;
        rotation.elements[1][3] = 0.0f;
        rotation.elements[2][3] = 0.0f;
        rotation.elements[3][3] = 1.0f;

        return rotation * m;
    }

    mat4 perspective(float fovy, float aspect, float near, float far) {
        mat4 result(0.0f);

        float tanHalfFovy = std::tan(fovy / 2.0f);

        result.elements[0][0] = 1.0f / (aspect * tanHalfFovy);
        result.elements[1][1] = 1.0f / tanHalfFovy;
        result.elements[2][2] = -(far + near) / (far - near);
        result.elements[2][3] = -1.0f;
        result.elements[3][2] = -(2.0f * far * near) / (far - near);

        return result;
    }

    float dot(const vec3& a, const vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    vec3 cross(const vec3& a, const vec3& b) {
        return vec3(a.y * b.z - a.z * b.y,
                    a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x);
    }

    mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
        vec3 f = (center - eye);
        float f_length = f.length();
        if (f_length < 1e-6f) {
            return mat4();
        }
        f = f / f_length;

        vec3 s = cross(f, up);
        float s_length = s.length();
        if (s_length < 1e-6f) {
            vec3 alternate_up = (std::abs(f.z) < 0.9f) ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
            s = cross(f, alternate_up);
            s_length = s.length();
        }
        s = s / s_length;

        vec3 u = cross(s, f);

        mat4 result;
        result.elements[0][0] = s.x;
        result.elements[0][1] = u.x;
        result.elements[0][2] = -f.x;

        result.elements[1][0] = s.y;
        result.elements[1][1] = u.y;
        result.elements[1][2] = -f.y;

        result.elements[2][0] = s.z;
        result.elements[2][1] = u.z;
        result.elements[2][2] = -f.z;

        result.elements[3][0] = -dot(s, eye);
        result.elements[3][1] = -dot(u, eye);
        result.elements[3][2] = dot(f, eye);

        return result;
    }

    quat quat_cast(const mat4& m) {
        float trace = m.elements[0][0] + m.elements[1][1] + m.elements[2][2];
        if (trace > 0) {
            float s = 0.5f / std::sqrt(trace + 1.0f);
            float w = 0.25f / s;
            float x = (m.elements[2][1] - m.elements[1][2]) * s;
            float y = (m.elements[0][2] - m.elements[2][0]) * s;
            float z = (m.elements[1][0] - m.elements[0][1]) * s;
            return quat(w, x, y, z);
        } else {
            if (m.elements[0][0] > m.elements[1][1] && m.elements[0][0] > m.elements[2][2]) {
                float s = 2.0f * std::sqrt(1.0f + m.elements[0][0] - m.elements[1][1] - m.elements[2][2]);
                float w = (m.elements[2][1] - m.elements[1][2]) / s;
                float x = 0.25f * s;
                float y = (m.elements[0][1] + m.elements[1][0]) / s;
                float z = (m.elements[0][2] + m.elements[2][0]) / s;
                return quat(w, x, y, z);
            } else if (m.elements[1][1] > m.elements[2][2]) {
                float s = 2.0f * std::sqrt(1.0f + m.elements[1][1] - m.elements[0][0] - m.elements[2][2]);
                float w = (m.elements[0][2] - m.elements[2][0]) / s;
                float x = (m.elements[0][1] + m.elements[1][0]) / s;
                float y = 0.25f * s;
                float z = (m.elements[1][2] + m.elements[2][1]) / s;
                return quat(w, x, y, z);
            } else {
                float s = 2.0f * std::sqrt(1.0f + m.elements[2][2] - m.elements[0][0] - m.elements[1][1]);
                float w = (m.elements[1][0] - m.elements[0][1]) / s;
                float x = (m.elements[0][2] + m.elements[2][0]) / s;
                float y = (m.elements[1][2] + m.elements[2][1]) / s;
                float z = 0.25f * s;
                return quat(w, x, y, z);
            }
        }
    }

    vec3 eulerAngles(const quat& q) {
        float norm = std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
        quat nq = (norm > 0.0f) ? quat(q.w / norm, q.x / norm, q.y / norm, q.z / norm) : quat();

        float roll = std::atan2(2.0f * (nq.w * nq.x + nq.y * nq.z), 1.0f - 2.0f * (nq.x * nq.x + nq.y * nq.y));
        float pitch = std::asin(2.0f * (nq.w * nq.y - nq.z * nq.x));
        float yaw = std::atan2(2.0f * (nq.w * nq.z + nq.x * nq.y), 1.0f - 2.0f * (nq.y * nq.y + nq.z * nq.z));

        return -vec3(roll, pitch, yaw);
    }

    float radians(float degrees) {
        return degrees * (M_PI / 180.0f);
    }

    float degrees(float radians) {
        return radians * (180.0f / M_PI);
    }

    float length(const vec3& v) {
        return v.length();
    }

    float* value_ptr(mat4& m) {
        return &m.elements[0][0];
    }

    float* value_ptr(vec3& v) {
        return &v.x;
    }

    float* value_ptr(const mat4& m){
        return (float*)&m.elements[0][0];
    }

    vec3 degrees(const vec3& v) {
        return vec3(degrees(v.x), degrees(v.y), degrees(v.z));
    }

    vec3 radians(const vec3& v) {
        return vec3(radians(v.x), radians(v.y), radians(v.z));
    }

    mat4 transpose(const mat4& m) {
        mat4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.elements[i][j] = m.elements[j][i];
            }
        }
        return result;
    }
}
