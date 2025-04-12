#pragma once

#include <myglm.h>
#include <array>
#include <vector>

using namespace myglm;

using Vertex = vec3;
using Triangle = u16vec3;
using Edge = u16vec2;

constexpr int gridSize = 1000;
constexpr int gridVertexCount = (2 * gridSize + 1) * 4;

static constexpr std::array<Vertex, gridVertexCount> generateGridVertices(int gridSize) {
    std::array<Vertex, gridVertexCount> vertices{};
    int index = 0;

    for (int i = -gridSize; i <= gridSize; ++i) {
        vertices[index++] = vec3(i, 0, -gridSize);
        vertices[index++] = vec3(i, 0, gridSize);
        vertices[index++] = vec3(-gridSize, 0, i);
        vertices[index++] = vec3(gridSize, 0, i);
    }
    return vertices;
}



struct Transform {
    quat q;
    vec3 translation;
    vec3 s;

    static Transform identity() {
        Transform transform{};
        transform.translation = vec3(0, 0, 0);
        transform.q = quat(1, 0, 0, 0);
        transform.s = vec3(1, 1, 1);
        return transform;
    }

    mat4 to_mat4() const {
        mat4 rotation_mat = rot_mat(q);
        mat4 translation_mat = translate(mat4(1.0f), translation);
        mat4 s_mat = scale(mat4(1.0f), s);
        return  s_mat  * rotation_mat * translation_mat;
    }
};

struct Object {
    std::string name;
    Transform transform;
    // std::vector<Vertex> vertices;
    // std::vector<Edge> edges;
    unsigned int VAO, VBO, EBO;
    unsigned int shader;
    unsigned int num_edges;
    unsigned int uid;

    virtual void draw(const mat4& projection, const mat4& view, bool selected) {
        mat4 model = transform.to_mat4();

        glUseProgram(shader);

        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(model));
        glUniform1i(glGetUniformLocation(shader, "u_selected"), selected);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glDrawElements(GL_LINES, num_edges * 2, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
    }

    virtual ~Object() {
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
    }
};

struct Torus : Object {
    float big_radius;
    float small_radius;
    unsigned int theta_samples;
    unsigned int phi_samples;

    Torus(
        float big_radius, float small_radius, unsigned int theta_samples, unsigned int phi_samples,
        const unsigned int shader, Transform transform = Transform::identity(), const std::string& name = "torus"
    )
    : big_radius(big_radius), small_radius(small_radius), theta_samples(theta_samples), phi_samples(phi_samples) {
        this->transform = transform;
        this->name = name;
        auto vertices = calc_vertices();
        auto edges = calc_edges();
        this->num_edges = edges.size();
        this->shader = shader;
        this->uid = 1;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(Edge), edges.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
    }

    [[nodiscard]] std::vector<Vertex> calc_vertices() const {
        std::vector<Vertex> vertices;
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

    [[nodiscard]] std::vector<Edge> calc_edges() const {
        std::vector<Edge> edges;
        edges.reserve(theta_samples * phi_samples * 6);

        for (unsigned int i = 0; i < theta_samples; ++i) {
            for (unsigned int j = 0; j < phi_samples; ++j) {
                unsigned int index1 = (i * (phi_samples + 1)) + j;
                unsigned int index2 = (i * (phi_samples + 1)) + j + 1;
                unsigned int index3 = ((i + 1) * (phi_samples + 1)) + j;
                unsigned int index4 = ((i + 1) * (phi_samples + 1)) + j + 1;

                edges.emplace_back(index1, index2);
                edges.emplace_back(index2, index3);
                edges.emplace_back(index3, index1);

                edges.emplace_back(index2, index4);
                edges.emplace_back(index4, index3);
                edges.emplace_back(index3, index2);
            }
        }
        return edges;
    }

};

struct Cursor : Object {
    Cursor(const unsigned int shader, const std::string& name = "cursor") {
        this->transform = Transform::identity();
        this->name = name;
        auto vertices = generateArrowVertices();
        auto edges = generateArrowEdges();
        this->num_edges = edges.size();
        this->shader = shader;
        this->uid = 0;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * 6 * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(Edge), edges.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    static std::array<float, 36> generateArrowVertices() {
        return {
            0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        };
    }

    static std::array<unsigned short, 6> generateArrowEdges() {
        return {
            0, 1,
            2, 3,
            4, 5
        };
    }

};

struct Point : Object {
    unsigned int samples;
    float radius;

    Point(const unsigned int shader, const float radius = 0.01f, const std::string& name = "point") {
        this->samples = 20;
        this->radius = radius;
        this->transform = Transform::identity();
        this->name = name;
        auto vertices = calc_vertices();
        auto edges = calc_edges();
        this->num_edges = edges.size();
        this->shader = shader;
        this->uid = 2;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(Edge), edges.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
    }

    [[nodiscard]] std::vector<Vertex> calc_vertices() const {
        std::vector<Vertex> vertices;
        vertices.reserve(samples * samples);

        for (unsigned int i = 0; i <= samples; ++i) {
            const float theta = 2.0f * M_PIf * static_cast<float>(i) / static_cast<float>(samples);
            for (unsigned int j = 0; j <= samples; ++j) {
                const float phi = M_PIf * static_cast<float>(j) / static_cast<float>(samples);

                float x = radius * cosf(theta) * sinf(phi);
                float y = radius * sinf(theta) * sinf(phi);
                float z = radius * cosf(phi);

                vertices.emplace_back(x, y, z);
            }
        }
        return vertices;
    }

    [[nodiscard]] std::vector<Edge> calc_edges() const {
        std::vector<Edge> edges;
        edges.reserve(samples * samples * 6);

        for (unsigned int i = 0; i < samples; ++i) {
            for (unsigned int j = 0; j < samples; ++j) {
                unsigned int index1 = (i * (samples + 1)) + j;
                unsigned int index2 = (i * (samples + 1)) + j + 1;
                unsigned int index3 = ((i + 1) * (samples + 1)) + j;
                unsigned int index4 = ((i + 1) * (samples + 1)) + j + 1;

                edges.emplace_back(index1, index2);
                edges.emplace_back(index2, index3);
                edges.emplace_back(index3, index1);

                edges.emplace_back(index2, index4);
                edges.emplace_back(index4, index3);
                edges.emplace_back(index3, index2);
            }
        }
        return edges;
    }

};

struct PolyLine : Object {
    std::vector<Point*> points;

    PolyLine(const unsigned int shader, const std::vector<Point*>& points, const std::string& name = "polyline") {
        this->points = points;
        this->transform = Transform::identity();
        this->name = name;
        this->shader = shader;
        auto vertices = calc_vertices();
        auto edges = calc_edges();
        this->num_edges = edges.size();
        this->uid = 3;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(Edge), edges.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
    }

    [[nodiscard]] std::vector<Vertex> calc_vertices() const {
        std::vector<Vertex> vertices;
        vertices.reserve(points.size());

        for (unsigned int i = 0; i < points.size(); ++i) {
            vertices.emplace_back(points[i]->transform.translation);
        }
        return vertices;
    }

    [[nodiscard]] std::vector<Edge> calc_edges() const {
        std::vector<Edge> edges;
        edges.reserve(points.size() - 1);

        for (unsigned int i = 0; i < points.size() - 1; ++i) {
            edges.emplace_back(i, i + 1);
        }
        return edges;
    }

    void draw(const mat4& projection, const mat4& view, bool selected) override {
        auto vertices = calc_vertices();
        auto edges = calc_edges();
        this->num_edges = edges.size();

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(Edge), edges.data(), GL_STATIC_DRAW);

        mat4 model = Transform::identity().to_mat4();

        glUseProgram(shader);

        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(model));
        glUniform1i(glGetUniformLocation(shader, "u_selected"), selected);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glDrawElements(GL_LINES, num_edges * 2, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
    }

};
