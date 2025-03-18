#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <chrono>
#include "glad.h"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "utility/shader_manager.h"
#include <geometry.h>
#include "debugging.h"
#include <myglm.h>

// window
GLFWwindow* window = nullptr;
int width = 1280, height = 720;

// mouse
double lastX = width / 2.0;
double lastY = height / 2.0;
bool leftMousePressed = false;
bool middleMousePressed = false;

// camera
myglm::vec3 target_position = myglm::vec3(0.0f, 0.0f, 0.0f);
float orbit_distance = 5.0f;
float orbit_yaw = 45.0f;
float orbit_pitch = 45.0f;
float mouseSensitivity = 0.1f;
float fov = 45.0f;
float zoomSensitivity = 0.1f;

// shaders manager
ShaderManager shader_manager({"../shaders/"});

// matrices
auto projection = myglm::mat4(1.0f);
auto view = myglm::mat4(1.0f);

// ImGui Variables
bool showObjectMenu = true;
bool showTorusTransform = false;

// FPS counter
std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
int frameCount = 0;
float fps = 0.0f;

// geometry
static constexpr auto gridVertices = generateGridVertices(gridSize);
ObjectRegistry object_registry(32, 32);
Object* selectedObject = nullptr;

// ImGui variables for torus properties
static constexpr myglm::vec3 defaultObjectPosition = { 0.0f, 0.0f, 0.0f };
static constexpr myglm::vec3 defaultObjectRotation = { 0.0f, 0.0f, 0.0f };
static constexpr myglm::vec3 defaultObjectScale = { 1.0f, 1.0f, 1.0f };
static constexpr float defaultTorusBigRadius = 1.0f;
static constexpr float defaultTorusSmallRadius = 0.25f;
static constexpr int defaultTorusThetaSamples = 10;
static constexpr int defaultTorusPhiSamples = 10;

myglm::vec3 torusPosition = defaultObjectPosition;
myglm::vec3 torusRotation = defaultObjectRotation;
myglm::vec3 torusScale = defaultObjectScale;
float torusBigRadius = defaultTorusBigRadius;
float torusSmallRadius = defaultTorusSmallRadius;
int torusThetaSamples = defaultTorusThetaSamples;
int torusPhiSamples = defaultTorusPhiSamples;

unsigned int tori_count = 0;

void framebuffer_size_callback(GLFWwindow* window_, int width_, int height_) {
    width = width_;
    height = height_;
    glViewport(0, 0, width_, height_);
}

void processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (leftMousePressed && !ImGui::GetIO().WantCaptureMouse) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;

        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        orbit_yaw += xoffset;
        orbit_pitch -= yoffset;

        if (orbit_pitch > 89.0f)
            orbit_pitch = 89.0f;
        if (orbit_pitch < -89.0f)
            orbit_pitch = -89.0f;

        lastX = xpos;
        lastY = ypos;
    }

    if (middleMousePressed && !ImGui::GetIO().WantCaptureMouse) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        float xoffset = xpos - lastX;
        float yoffset = ypos - lastY;

        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        myglm::vec3 camera_direction = myglm::normalize(target_position - myglm::vec3(
            orbit_distance * cos(myglm::radians(orbit_yaw)) * cos(myglm::radians(orbit_pitch)),
            orbit_distance * sin(myglm::radians(orbit_pitch)),
            orbit_distance * sin(myglm::radians(orbit_yaw)) * cos(myglm::radians(orbit_pitch))
        ));

        myglm::vec3 camera_right = myglm::normalize(myglm::cross(myglm::vec3(0.0f, 1.0f, 0.0f), camera_direction));
        myglm::vec3 camera_up = myglm::normalize(myglm::cross(camera_direction, camera_right));

        target_position = target_position + camera_right * xoffset * orbit_distance * 0.01f;
        target_position = target_position + camera_up * yoffset * orbit_distance * 0.01f;

        lastX = xpos;
        lastY = ypos;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        leftMousePressed = action == GLFW_PRESS;
        if (leftMousePressed) {
            glfwGetCursorPos(window, &lastX, &lastY);
            if (!ImGui::GetIO().WantCaptureMouse) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        middleMousePressed = action == GLFW_PRESS;
        if (middleMousePressed) {
            glfwGetCursorPos(window, &lastX, &lastY);
            if (!ImGui::GetIO().WantCaptureMouse) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        } else {
            if (!leftMousePressed)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    orbit_distance -= (float)yoffset * zoomSensitivity;
    if (orbit_distance < 1.0f)
        orbit_distance = 1.0f;
}

void render_gui() {
    ImGui::Begin("Objects", &showObjectMenu, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // Object Creation
    if (ImGui::Button("Torus")) {
        selectedObject = object_registry.register_object(
            new Torus(torusSmallRadius, torusBigRadius, torusPhiSamples, torusThetaSamples)
        );
        selectedObject->name = "Torus" + std::to_string(tori_count);
        torusPosition = defaultObjectPosition;
        torusRotation = defaultObjectRotation;
        torusScale = defaultObjectScale;
        torusBigRadius = defaultTorusBigRadius;
        torusSmallRadius = defaultTorusSmallRadius;
        torusThetaSamples = defaultTorusThetaSamples;
        torusPhiSamples = defaultTorusPhiSamples;
        tori_count++;
    }

    // Object List
    if (ImGui::TreeNode("Object List")) {
        for (auto& obj : object_registry.objects) {
            bool is_selected = (obj == selectedObject);
            if (ImGui::Selectable(obj->name.c_str(), is_selected)) {
                selectedObject = obj;

                if (Torus* torus = dynamic_cast<Torus*>(selectedObject)) {
                    torusPosition = myglm::vec3(torus->transform[3]);
                    myglm::quat rotation_quat = myglm::quat_cast(torus->transform);
                    torusRotation = myglm::eulerAngles(rotation_quat);
                    torusRotation = myglm::degrees(torusRotation);
                    torusScale = myglm::vec3(myglm::length(myglm::vec3(torus->transform[0])), myglm::length(myglm::vec3(torus->transform[1])), myglm::length(myglm::vec3(torus->transform[2])));
                    torusBigRadius = torus->big_radius;
                    torusSmallRadius = torus->small_radius;
                    torusThetaSamples = torus->theta_samples;
                    torusPhiSamples = torus->phi_samples;
                }
            }
        }
        ImGui::TreePop();
    }

    // Object Properties
    if (selectedObject != nullptr) {
        if (ImGui::TreeNode("Object Properties")) {
            if (Torus* torus = dynamic_cast<Torus*>(selectedObject)) {
                ImGui::DragFloat3("Position", myglm::value_ptr(torusPosition), 0.1f);
                ImGui::DragFloat3("Rotation", myglm::value_ptr(torusRotation), 1.0f);
                ImGui::DragFloat3("Scale", myglm::value_ptr(torusScale), 0.1f);
                ImGui::DragFloat("Big Radius", &torusBigRadius, 0.05f, 0.01f);
                if (torusBigRadius < 0.01f) {
                    torusBigRadius = 0.01f;
                }
                ImGui::DragFloat("Small Radius", &torusSmallRadius, 0.05f, 0.01f);
                if (torusSmallRadius < 0.01f) {
                    torusSmallRadius = 0.01f;
                }
                ImGui::DragInt("Theta Samples", &torusThetaSamples, 1, 3);
                if (torusThetaSamples < 3) {
                    torusThetaSamples = 3;
                }
                ImGui::DragInt("Phi Samples", &torusPhiSamples, 1, 3);
                if (torusPhiSamples < 3) {
                    torusPhiSamples = 3;
                }

                auto model = myglm::mat4(1.0f);

                model = myglm::translate(model, torusPosition);

                model = myglm::rotate(model, myglm::radians(torusRotation.x), myglm::vec3(1.0f, 0.0f, 0.0f));
                model = myglm::rotate(model, myglm::radians(torusRotation.y), myglm::vec3(0.0f, 1.0f, 0.0f));
                model = myglm::rotate(model, myglm::radians(torusRotation.z), myglm::vec3(0.0f, 0.0f, 1.0f));

                model = myglm::scale(model, torusScale);

                torus->transform = model;

                torus->big_radius = torusBigRadius;
                torus->small_radius = torusSmallRadius;
                torus->theta_samples = torusThetaSamples;
                torus->phi_samples = torusPhiSamples;
            }
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
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(myglm::vec3), gridVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void render_grid() {
    unsigned int shaderProgram = shader_manager.shader_program({"grid"});

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, myglm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, myglm::value_ptr(view));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertices.size());
    glBindVertexArray(0);

    glDeleteProgram(shaderProgram);
}

void render_mesh(const std::pair<Vertices, Triangles>& buffers, const std::string& shader_name, const myglm::mat4& model) {
    unsigned int shaderProgram = shader_manager.shader_program({shader_name});

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, myglm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, myglm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, myglm::value_ptr(model));

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

    glDeleteProgram(shaderProgram);
}

void render_wireframe(const std::pair<Vertices, Lines>& buffers, const std::string& shader_name, const myglm::mat4& model) {
    unsigned int shaderProgram = shader_manager.shader_program({shader_name});

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, myglm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, myglm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, myglm::value_ptr(model));

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, buffers.first.size() * sizeof(VertexFormat), buffers.first.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffers.second.size() * sizeof(LineFormat), buffers.second.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, buffers.second.size() * 2, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glDeleteProgram(shaderProgram);
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
        projection = myglm::perspective(myglm::radians(fov),  ascpet_ratio, 0.1f, orbit_distance * 10.0f);

        myglm::vec3 camera_position = target_position + myglm::vec3(
            orbit_distance * cos(myglm::radians(orbit_yaw)) * cos(myglm::radians(orbit_pitch)),
            orbit_distance * sin(myglm::radians(orbit_pitch)),
            orbit_distance * sin(myglm::radians(orbit_yaw)) * cos(myglm::radians(orbit_pitch))
        );

        view = myglm::lookAt(camera_position, target_position, myglm::vec3(0.0f, 1.0f, 0.0f));

        render_grid();

        for (const auto obj : object_registry.objects) {
            const bool wireframe = obj->wireframe;
            const auto shader_name = obj->shader();
            const auto model = obj->transform;

            if (wireframe) {
                const auto buffers = object_registry.get_wireframe(obj);
                render_wireframe(buffers, shader_name, model);
            } else {
                const auto buffers = object_registry.get_mesh(obj);
                render_mesh(buffers, shader_name, model);
            }
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
