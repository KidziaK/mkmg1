#include <array>
#include <glm/glm.hpp>
#include <functional>
#include <unordered_map>

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

struct Object {
    virtual ~Object() = default;

    glm::mat4 transform = glm::mat4(1.0f);

    [[nodiscard]] virtual std::string shader() const = 0;
    [[nodiscard]] virtual std::vector<VertexFormat> vertices() const = 0;
    [[nodiscard]] virtual std::vector<IndexFormat> triangles() const = 0;
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
    size_t hash() const override {
        const size_t h1 = std::hash<int>{}(std::bit_cast<int>(big_radius));
        const size_t h2 = std::hash<int>{}(std::bit_cast<int>(small_radius));
        const size_t h3 = std::hash<unsigned int>{}(theta_samples);
        const size_t h4 = std::hash<unsigned int>{}(phi_samples);
        const size_t combinedHash = h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        return combinedHash;
    }

    Torus() = default;

    Torus(float r, float R, int s_phi, int s_theta) : small_radius(r), big_radius(R), theta_samples(s_phi), phi_samples(s_theta) {}
};

using Vertices = std::vector<VertexFormat>;
using Triangles = std::vector<IndexFormat>;

struct ObjectRegistry {
    std::vector<Object*> objects;
    std::unordered_map<size_t, std::pair<Vertices, Triangles>> buffers;
    std::unordered_map<size_t, unsigned char> counts;

    std::pair<Vertices, Triangles> get_buffers(Object* obj) {
        size_t hash = obj->hash();

        if (buffers.contains(hash)) {
            return buffers.at(hash);
        }

        auto vertices = obj->vertices();
        auto triangles = obj->triangles();
        buffers.emplace(hash, std::make_pair(vertices, triangles));

        return {vertices, triangles};
    }
};
