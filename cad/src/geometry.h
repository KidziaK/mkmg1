#pragma once

#ifdef DEBUG
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#else
#include <myglm.h>
#endif

#include <array>
#include <functional>
#include "lru_cache.h"

constexpr int gridSize = 1000;
constexpr int gridVertexCount = (2 * gridSize + 1) * 4;

static constexpr std::array<glm::vec3, gridVertexCount> generateGridVertices(int gridSize) {
    std::array<glm::vec3, gridVertexCount> vertices{};
    int index = 0;

    for (int i = -gridSize; i <= gridSize; ++i) {
        vertices[index++] = glm::vec3(i, 0, -gridSize);
        vertices[index++] = glm::vec3(i, 0, gridSize);
        vertices[index++] = glm::vec3(-gridSize, 0, i);
        vertices[index++] = glm::vec3(gridSize, 0, i);
    }
    return vertices;
}

using VertexFormat = glm::vec3;
using IndexFormat = glm::u16vec3;
using LineFormat = glm::u16vec2;

struct Object {
    virtual ~Object() = default;

    bool wireframe = true;

    glm::mat4 transform = glm::mat4(1.0f);

    std::string name;

    [[nodiscard]] virtual std::string shader() const = 0;
    [[nodiscard]] virtual std::vector<VertexFormat> vertices() const = 0;
    [[nodiscard]] virtual std::vector<IndexFormat> triangles() const = 0;
    [[nodiscard]] virtual std::vector<LineFormat> lines() const = 0;
    [[nodiscard]] virtual size_t hash() const = 0;
};

struct Torus final : Object {
    float big_radius;
    float small_radius;
    unsigned int theta_samples;
    unsigned int phi_samples;

    [[nodiscard]]
    std::string shader() const override {
        return "torus";
    }

    [[nodiscard]]
    std::vector<VertexFormat> vertices() const override {
        std::vector<VertexFormat> vertices;
        vertices.reserve(theta_samples * phi_samples);

        for (unsigned int i = 0; i <= theta_samples; ++i) {
            const float theta = 2.0f * M_PIf * static_cast<float>(i) / static_cast<float>(theta_samples);
            for (unsigned int j = 0; j <= phi_samples; ++j) {
                const float phi = 2.0f * M_PIf * static_cast<float>(j) / static_cast<float>(phi_samples);

                float x = (big_radius + small_radius * cosf(phi)) * cosf(theta);
                float y = (big_radius + small_radius * cosf(phi)) * sinf(theta);
                float z = small_radius * sinf(phi);

                vertices.emplace_back(x, y, z);
            }
        }
        return vertices;
    }

    [[nodiscard]]
    std::vector<IndexFormat> triangles() const override {
        std::vector<IndexFormat> triangles;
        triangles.reserve(theta_samples * phi_samples * 2);

        for (unsigned int i = 0; i < theta_samples; ++i) {
            for (unsigned int j = 0; j < phi_samples; ++j) {
                unsigned int index1 = (i * (phi_samples + 1)) + j;
                unsigned int index2 = (i * (phi_samples + 1)) + j + 1;
                unsigned int index3 = ((i + 1) * (phi_samples + 1)) + j;
                unsigned int index4 = ((i + 1) * (phi_samples + 1)) + j + 1;

                triangles.emplace_back(index1, index2, index3);
                triangles.emplace_back(index2, index4, index3);
            }
        }
        return triangles;
    }

    [[nodiscard]]
    std::vector<LineFormat> lines() const override {
        std::vector<LineFormat> lines;
        lines.reserve(theta_samples * phi_samples * 6);

        for (unsigned int i = 0; i < theta_samples; ++i) {
            for (unsigned int j = 0; j < phi_samples; ++j) {
                unsigned int index1 = (i * (phi_samples + 1)) + j;
                unsigned int index2 = (i * (phi_samples + 1)) + j + 1;
                unsigned int index3 = ((i + 1) * (phi_samples + 1)) + j;
                unsigned int index4 = ((i + 1) * (phi_samples + 1)) + j + 1;

                lines.emplace_back(index1, index2);
                lines.emplace_back(index2, index3);
                lines.emplace_back(index3, index1);

                lines.emplace_back(index2, index4);
                lines.emplace_back(index4, index3);
                lines.emplace_back(index3, index2);
            }
        }
        return lines;
    }

    [[nodiscard]]
    size_t hash() const override {
        size_t seed = 0;

        std::string typeName = typeid(*this).name();
        for (char c : typeName) {
            seed ^= std::hash<char>{}(c) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        seed ^= std::hash<float>{}(big_radius) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<float>{}(small_radius) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<unsigned int>{}(theta_samples) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<unsigned int>{}(phi_samples) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }

    Torus() = default;

    Torus(float r, float R, int s_phi, int s_theta) : small_radius(r), big_radius(R), theta_samples(s_phi), phi_samples(s_theta) {}
};

using Vertices = std::vector<VertexFormat>;
using Triangles = std::vector<IndexFormat>;
using Lines = std::vector<LineFormat>;

struct ObjectRegistry {
    std::vector<Object*> objects;
    LRUCache<size_t, std::pair<Vertices, Triangles>> mesh_buffers;
    LRUCache<size_t, std::pair<Vertices, Lines>> wireframe_buffers;

    ObjectRegistry(size_t mesh_capacity, size_t wireframe_capacity) : mesh_buffers(mesh_capacity), wireframe_buffers(wireframe_capacity) {}

    Object* register_object(Object* obj) {
        objects.emplace_back(obj);
        return obj;
    }

    std::pair<Vertices, Triangles> get_mesh(Object* obj) {
        size_t hash = obj->hash();
        auto mesh = mesh_buffers.get(hash);
        if (mesh.first.empty() && mesh.second.empty()) {
            auto vertices = obj->vertices();
            auto triangles = obj->triangles();
            mesh_buffers.put(hash, {vertices, triangles});
            return {vertices, triangles};
        }
        return mesh;
    }

    std::pair<Vertices, Lines> get_wireframe(Object* obj) {
        const size_t hash = obj->hash();
        auto wireframe = wireframe_buffers.get(hash);
        if (wireframe.first.empty() && wireframe.second.empty()) {
            auto vertices = obj->vertices();
            auto lines = obj->lines();
            wireframe_buffers.put(hash, {vertices, lines});
            return {vertices, lines};
        }
        return wireframe;
    }
};

inline std::pair<Vertices, Triangles> generateRotationBall(float radius, int slices, int stacks) {
    std::vector<VertexFormat> vertices;
    std::vector<IndexFormat> indices;

    for (int i = 0; i <= stacks; ++i) {
        float v = static_cast<float>(i) / static_cast<float>(stacks);
        float phi = v * M_PIf;

        for (int j = 0; j <= slices; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(slices);
            float theta = u * 2.0f * M_PIf;

            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);

            vertices.emplace_back(x, y, z);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = (i * (slices + 1)) + j;
            int second = first + slices + 1;

            indices.emplace_back(first, second, first + 1);
            indices.emplace_back(second, second + 1, first + 1);
        }
    }
    return {vertices, indices};
}
