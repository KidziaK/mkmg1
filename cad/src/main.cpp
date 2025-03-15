#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <chrono>
#include "glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "utility/shader_manager.h"
#include "callbacks_and_io.h"
#include <geometry.h>

// window
GLFWwindow* window = nullptr;
int width = 1280, height = 720;

// mouse
double lastX = width / 2.0;
double lastY = height / 2.0;
bool leftMousePressed = false;
bool middleMousePressed = false;

// camera
glm::vec3 target_position = glm::vec3(0.0f, 0.0f, 0.0f);
float orbit_distance = 5.0f;
float orbit_yaw = -45.0f;
float orbit_pitch = 45.0f;
float mouseSensitivity = 0.1f;
float fov = 45.0f;
float zoomSensitivity = 0.1f;

// shaders manager
ShaderManager shader_manager({"../shaders/"});

// matrices
auto projection = glm::mat4(1.0f);
auto view = glm::mat4(1.0f);

// ImGui Variables
bool showObjectMenu = true;
bool showTorusTransform = false;

// FPS counter
std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
int frameCount = 0;
float fps = 0.0f;

// geometry
static constexpr auto gridVertices = generateGridVertices(gridSize);
ObjectRegistry object_registry;

void render_gui() {
    ImGui::Begin("Objects", &showObjectMenu, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    if (ImGui::Button("Torus")) {
        showTorusTransform = true;
    }

    if (showTorusTransform) {
        if (ImGui::TreeNode("Torus Transform")) {
            static float torusPosition[3] = { 0.0f, 0.0f, 0.0f };
            static float torusRotation[3] = { 0.0f, 0.0f, 0.0f };
            static float torusScale[3] = { 1.0f, 1.0f, 1.0f };

            ImGui::DragFloat3("Position", torusPosition, 0.1f);
            ImGui::DragFloat3("Rotation", torusRotation, 1.0f);
            ImGui::DragFloat3("Scale", torusScale, 0.1f);

            ImGui::TreePop();
        }
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::End();
}

unsigned int gridVAO, gridVBO;

void initializeGridBuffers() {
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(glm::vec3), gridVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void render_grid() {
    unsigned int shaderProgram = shader_manager.shader_program({"grid"});

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertices.size());
    glBindVertexArray(0);
}

void render_object(const std::pair<Vertices, Triangles>& buffers, const std::string& shader_name) {
    unsigned int shaderProgram = shader_manager.shader_program({shader_name});

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    // Here we should get model matrix from object and pass it to shader.
    // However, the object_registry does not store model matrix currently.
    // For now we pass identity matrix.
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, buffers.first.size() * sizeof(VertexFormat), buffers.first.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffers.second.size() * sizeof(IndexFormat), buffers.second.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, buffers.second.size() * 3, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "MKMG1", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glEnable(GL_DEPTH_TEST);

    initializeGridBuffers();

    lastTime = std::chrono::high_resolution_clock::now();

    object_registry.objects.emplace_back(new Torus(0.1f, 1.01f, 36, 36));

    while (!glfwWindowShouldClose(window)) {
        processInput();
        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int imguiWindowWidth = 250;
        glViewport(0, 0, width - imguiWindowWidth, height);

        float ascpet_ratio = static_cast<float>(width - imguiWindowWidth) / static_cast<float>(height);
        projection = glm::perspective(glm::radians(fov),  ascpet_ratio, 0.1f, orbit_distance * 10.0f);

        glm::vec3 camera_position = target_position + glm::vec3(
            orbit_distance * cos(glm::radians(orbit_yaw)) * cos(glm::radians(orbit_pitch)),
            orbit_distance * sin(glm::radians(orbit_pitch)),
            orbit_distance * sin(glm::radians(orbit_yaw)) * cos(glm::radians(orbit_pitch))
        );

        view = glm::lookAt(camera_position, target_position, glm::vec3(0.0f, 1.0f, 0.0f));

        render_grid();

        for (const auto obj : object_registry.objects) {
            const auto buffers = object_registry.get_buffers(obj);
            const auto shader_name = obj->shader();

            render_object(buffers, shader_name);
        }

        ImGui::SetNextWindowPos(ImVec2(width - imguiWindowWidth, 0));
        ImGui::SetNextWindowSize(ImVec2(imguiWindowWidth, height));

        render_gui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glViewport(0, 0, width, height);

        glfwSwapBuffers(window);
        glfwPollEvents();

        frameCount++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        if (deltaTime >= 0.1f) {
            fps = frameCount / deltaTime;
            frameCount = 0;
            lastTime = currentTime;
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}