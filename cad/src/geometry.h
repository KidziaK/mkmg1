#pragma once

#include <myglm.h>
#include <array>
#include <unordered_set>
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
    vec3 rotation;
    vec3 translation;
    vec3 s;

    static Transform identity() {
        Transform transform{};
        transform.translation = vec3(0, 0, 0);
        transform.rotation = vec3(0, 0, 0);
        transform.s = vec3(1, 1, 1);
        return transform;
    }

    mat4 to_mat4() const {
        quat q = from_euler_angles(rotation);
        mat4 rotation_mat = rot_mat(q);
        mat4 translation_mat = translate(mat4(1.0f), translation);
        mat4 s_mat = scale(mat4(1.0f), s);
        return s_mat * rotation_mat * translation_mat;
    }

    static Transform from_mat4(const mat4& matrix) {
        Transform result = identity();

        result.translation.x = matrix.elements[3][0];
        result.translation.y = matrix.elements[3][1];
        result.translation.z = matrix.elements[3][2];

        vec3 column_x(matrix.elements[0][0], matrix.elements[0][1], matrix.elements[0][2]);
        vec3 column_y(matrix.elements[1][0], matrix.elements[1][1], matrix.elements[1][2]);
        vec3 column_z(matrix.elements[2][0], matrix.elements[2][1], matrix.elements[2][2]);

        result.s.x = column_x.length();
        result.s.y = column_y.length();
        result.s.z = column_z.length();

        mat3 rotation_matrix = mat3(1.0f);
        rotation_matrix.elements[0][0] = matrix.elements[0][0] / result.s.x;
        rotation_matrix.elements[1][0] = matrix.elements[1][0] / result.s.y;
        rotation_matrix.elements[2][0] = matrix.elements[2][0] / result.s.z;

        rotation_matrix.elements[0][1] = matrix.elements[0][1] / result.s.x;
        rotation_matrix.elements[1][1] = matrix.elements[1][1] / result.s.y;
        rotation_matrix.elements[2][1] = matrix.elements[2][1] / result.s.z;

        rotation_matrix.elements[0][2] = matrix.elements[0][2] / result.s.x;
        rotation_matrix.elements[1][2] = matrix.elements[1][2] / result.s.y;
        rotation_matrix.elements[2][2] = matrix.elements[2][2] / result.s.z;

        quat rotation_quat(mat4_cast(rotation_matrix));
        rotation_quat.normalize();
        result.rotation = eulerAngles(rotation_quat);

        return result;
    }
};

struct Object {
    std::string name;
    Transform transform;
    unsigned int VAO, VBO, EBO;
    unsigned int shader;
    unsigned int num_edges;
    unsigned int uid;

    virtual void draw(const mat4& projection, const mat4& view, bool selected, const mat4& global_transform) {
        mat4 model = transform.to_mat4();

        glUseProgram(shader);

        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(model * global_transform));
        glUniform1i(glGetUniformLocation(shader, "u_selected"), selected);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glDrawElements(GL_LINES, num_edges * 2, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
    }

    virtual void update(
        const mat4& global_transform,
        const std::unordered_set<Object*>& selected_objects,
        const mat4& projection,
        const mat4& view,
        unsigned int width,
        unsigned int height
    ) {}

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
        this->uid = 3;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
    }

    [[nodiscard]] std::vector<Vertex> calc_vertices(const mat4& global_transform, const std::unordered_set<Object*>& selected_objects) const {
        std::vector<Vertex> vertices;
        vertices.reserve(points.size());

        for (unsigned int i = 0; i < points.size(); ++i) {
            mat4 modified_transform = selected_objects.contains(points[i]) ? global_transform : mat4(1.0f);
            vec4 t_affine = vec4(points[i]->transform.translation, 1.0f);
            vec3 t = vec3_from_vec4(mul(modified_transform, t_affine));
            vertices.emplace_back(t);
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

    void update(
        const mat4& global_transform,
        const std::unordered_set<Object*>& selected_objects,
        const mat4& projection,
        const mat4& view,
        unsigned int width,
        unsigned int height
    ) override {
        auto vertices = calc_vertices(global_transform, selected_objects);
        auto edges = calc_edges();
        this->num_edges = edges.size();

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(Edge), edges.data(), GL_STATIC_DRAW);

        transform = Transform::identity();
    }

    void draw(const mat4& projection, const mat4& view, bool selected, const mat4& global_transform) override {
        Object::draw(projection, view, selected, mat4(1.0f));
    }
};

struct C0Bezier : Object {
    std::vector<Point*> control_points;
    PolyLine* control_polygon;
    std::vector<vec3> curve_vertices;
    bool show_control_polygon = true;

    C0Bezier(const unsigned int shader, const std::vector<Point*>& control_points, const std::string& name = "C0 Bezier") {
        this->control_points = control_points;
        this->transform = Transform::identity();
        this->name = name;
        this->shader = shader;
        this->uid = 4;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        this->control_polygon = new PolyLine(shader, control_points);
    }

    [[nodiscard]] std::vector<vec3> calc_vertices(
        const std::vector<vec3>& modified_control_points,
        const mat4& projection,
        const mat4& view,
        unsigned int width,
        unsigned int height
    ) const {
        std::vector<vec3> vertices;

        vertices.reserve(modified_control_points.size());

        for (unsigned int i = 0; i < modified_control_points.size() - 1; i+=3) {
            vec3 p0 = modified_control_points[i];
            vec3 p1 = modified_control_points[i + 1];
            vec3 p2 = modified_control_points[i + 2];
            vec3 p3 = modified_control_points[i + 3];

            vec4 q0 = mul(projection * view, vec4(p0, 1.0f));
            vec4 q1 = mul(projection * view, vec4(p1, 1.0));
            vec4 q2 = mul(projection * view, vec4(p2, 1.0));
            vec4 q3 = mul(projection * view, vec4(p3, 1.0));

            float xMax = std::max({q0.x, q1.x, q2.x, q3.x});
            float yMax = std::max({q0.y, q1.y, q2.y, q3.y});

            float xMin = std::min({q0.x, q1.x, q2.x, q3.x});
            float yMin = std::min({q0.y, q1.y, q2.y, q3.y});

            unsigned int points_per_segment = (yMax - yMin) * (xMax - xMin) * 100;
            points_per_segment = std::max(int(points_per_segment), 2);

            for (unsigned int j = 0; j < points_per_segment; ++j) {
                float t = static_cast<float>(j) / static_cast<float>(points_per_segment - 1);
                vertices.emplace_back(bezierPoint(t, p0, p1, p2, p3));
            }
        }
        return vertices;
    }

    [[nodiscard]] std::vector<u16vec2> calc_edges() const {
        std::vector<u16vec2> edges;

        edges.reserve(curve_vertices.size());

        for (int i = 0; i < curve_vertices.size() - 1; ++i) {
            edges.emplace_back(i, i + 1);
        }

        return edges;
    }

    void update(
        const mat4& global_transform,
        const std::unordered_set<Object*>& selected_objects,
        const mat4& projection,
        const mat4& view,
        unsigned int width,
        unsigned int height
    ) override {
        std::vector<vec3> modified_control_points;
        modified_control_points.reserve(control_points.size() + 3);

        for (unsigned int i = 0; i < control_points.size(); ++i) {
            mat4 transform = selected_objects.contains(control_points[i]) ? global_transform : mat4(1.0f);
            vec3 transformed_point = vec3_from_vec4(mul(transform, vec4(control_points[i]->transform.translation, 1.0f)));
            modified_control_points.emplace_back(transformed_point);
        }

        const unsigned int n = modified_control_points.size();

        unsigned int k = n <= 4 ? 4 - n : (4 - n) % 3;

        for (unsigned int i = 0; i < k; ++i) {
            modified_control_points[n + i] = modified_control_points[n - 1];
        }

        curve_vertices = calc_vertices(modified_control_points, projection, view, width, height);
        auto edges = calc_edges();
        this->num_edges = edges.size();

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, curve_vertices.size() * sizeof(Vertex), curve_vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(Edge), edges.data(), GL_STATIC_DRAW);

        transform = Transform::identity();

        if (show_control_polygon) {
            control_polygon->update(global_transform, selected_objects, projection, view, width, height);
        }
    }

    void draw(const mat4& projection, const mat4& view, bool selected, const mat4& global_transform) override {
        Object::draw(projection, view, selected, mat4(1.0f));
        if (show_control_polygon) {
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            control_polygon->draw(projection, view, selected, mat4(1.0f));
        }
    }

    ~C0Bezier() override {
        delete control_polygon;
        Object::~Object();
    }
};