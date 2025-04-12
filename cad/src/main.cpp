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
#include <unordered_set>

using namespace myglm;

// window
GLFWwindow* window = nullptr;
int width = 1920, height = 1080;

// mouse
double lastX = width / 2.0;
double lastY = height / 2.0;
bool leftMousePressed = false;
bool middleMousePressed = false;

// camera
vec3 target_position = vec3(0.0f, 0.0f, 0.0f);
float orbit_distance = 5.0f;
float orbit_yaw = 45.0f;
float orbit_pitch = 45.0f;
float mouseSensitivity = 0.1f;
float fov = 45.0f;
float zoomSensitivity = 0.1f;
vec3 camera_position;

// shaders manager
ShaderManager shader_manager({"../shaders/"});
unsigned int grid_shader_program;
unsigned int torus_shader;
unsigned int cursor_shader;
unsigned int point_shader;

// matrices
auto projection = mat4(1.0f);
auto view = mat4(1.0f);

// ImGui Variables
bool showOptionsMenu = true;
bool showTransformMenu = false;

// FPS counter
std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
int frameCount = 0;
float fps = 0.0f;

// static objects
static constexpr auto gridVertices = generateGridVertices(gridSize);

// dynamic objects
std::vector<Object*> objects = {};
std::unordered_set<Object*> selected_objects;

// transform window
float transform_window_trans[3] = {0, 0, 0};
float transform_window_rot[3] = {0, 0, 0};
float transform_window_scale[3] = {1, 1, 1};

// torus
float big_radius_menu;
float small_radius_menu;
int theta_samples_menu;
int phi_samples_menu;

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

        vec3 camera_direction = normalize(target_position - vec3(
            orbit_distance * cosf(radians(orbit_yaw)) * cosf(radians(orbit_pitch)),
            orbit_distance * sinf(radians(orbit_pitch)),
            orbit_distance * sinf(radians(orbit_yaw)) * cosf(radians(orbit_pitch))
        ));

        vec3 camera_right = normalize(cross(vec3(0.0f, 1.0f, 0.0f), camera_direction));
        vec3 camera_up = normalize(cross(camera_direction, camera_right));

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

void render_fps_counter() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::End();
}

void add_torus(Torus* torus, bool in_cursor = true, bool select = false) {
    objects.push_back(torus);
    if (in_cursor) {
        torus->transform = objects[0]->transform;
    }
    if (select) {
        selected_objects.clear();
        selected_objects.insert(torus);
    }
}

void render_options_menu() {
    ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoCollapse);
    if (ImGui::Button("Torus")) {
        auto ptorus = new Torus(1.0f, 0.1f, 25, 25, torus_shader);
        add_torus(ptorus);
    }

    if (ImGui::Button("Point")) {
        auto ppoint = new Point(point_shader);
        objects.push_back(ppoint);
        ppoint->transform = objects[0]->transform;
    }

    ImGui::End();
}

void render_single_object_transform_menu() {
    ImGui::Begin("Transform", &showTransformMenu);
    auto& selected_obj = *selected_objects.begin();

    transform_window_trans[0] = selected_obj->transform.translation.x;
    transform_window_trans[1] = selected_obj->transform.translation.y;
    transform_window_trans[2] = selected_obj->transform.translation.z;

    transform_window_scale[0] = selected_obj->transform.s.x;
    transform_window_scale[1] = selected_obj->transform.s.y;
    transform_window_scale[2] = selected_obj->transform.s.z;

    vec3 euler_angles = eulerAngles(selected_obj->transform.q);

    transform_window_rot[0] = degrees(euler_angles.x);
    transform_window_rot[1] = degrees(euler_angles.y);
    transform_window_rot[2] = degrees(euler_angles.z);

    if (ImGui::SliderFloat3("translation", transform_window_trans, -5.0f, 5.0f) ) {
        selected_obj->transform.translation.x = transform_window_trans[0];
        selected_obj->transform.translation.y = transform_window_trans[1];
        selected_obj->transform.translation.z = transform_window_trans[2];
    }
    if (ImGui::SliderFloat3("rotation", transform_window_rot, -89.99, 89.99) ) {
        float rad_x = radians(transform_window_rot[0]);
        float rad_y = radians(transform_window_rot[1]);
        float rad_z = radians(transform_window_rot[2]);

        quat quat_x = angleAxis(rad_x, vec3(1, 0, 0));
        quat quat_y = angleAxis(rad_y, vec3(0, 1, 0));
        quat quat_z = angleAxis(rad_z, vec3(0, 0, 1));

        quat final_rotation = quat_z * quat_y * quat_x;

        selected_obj->transform.q = final_rotation;
    }
    if (ImGui::SliderFloat3("scale", transform_window_scale, 0.1f, 5.0f) ) {
        selected_obj->transform.s.x = transform_window_scale[0];
        selected_obj->transform.s.y = transform_window_scale[1];
        selected_obj->transform.s.z = transform_window_scale[2];
    }
    ImGui::End();
}

void render_objects_list_window() {
    ImGui::Begin("Objects", nullptr, ImGuiWindowFlags_NoCollapse);
    for (int i = 0; i < objects.size(); ++i) {
        auto& obj = objects[i];
        std::string& item_name = obj->name;
        char buffer[256];
        strcpy(buffer, item_name.c_str());

        ImGui::PushID(i); // Unique ID for each item

        if (ImGui::InputText("", buffer, IM_ARRAYSIZE(buffer))) {
            obj->name = buffer;
        }

        if (ImGui::IsItemClicked()) {
            selected_objects.clear();
            selected_objects.insert(objects[i]);
        }

        if (item_name != "cursor") {
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                delete obj;
                objects.erase(objects.begin() + i);
                selected_objects.clear();
            }
        }

        ImGui::PopID();
    }

    ImGui::End();
}

void render_torus_menu() {
    ImGui::Begin("Torus", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& basePtr = *selected_objects.begin();
    Torus* obj = dynamic_cast<Torus*>(basePtr);

    big_radius_menu = obj->big_radius;
    small_radius_menu = obj->small_radius;
    theta_samples_menu = obj->theta_samples;
    phi_samples_menu = obj->phi_samples;

    if (ImGui::SliderFloat("R", &big_radius_menu, 0.1f, 5.0f) ) {
        auto ptorus = new Torus(big_radius_menu, obj->small_radius, obj->theta_samples, obj->phi_samples, torus_shader, obj->transform, obj->name);
        objects.erase(std::remove(objects.begin(), objects.end(), obj), objects.end());
        delete obj;
        selected_objects.erase(obj);
        add_torus(ptorus, false, true);
    }

    if (ImGui::SliderFloat("r", &small_radius_menu, 0.1f, 5.0f) ) {
        auto ptorus = new Torus(obj->big_radius, small_radius_menu, obj->theta_samples, obj->phi_samples, torus_shader, obj->transform, obj->name);
        objects.erase(std::remove(objects.begin(), objects.end(), obj), objects.end());
        delete obj;
        selected_objects.erase(obj);
        add_torus(ptorus, false, true);
    }

    if (ImGui::SliderInt("theta", &theta_samples_menu, 3, 100) ) {
        auto ptorus = new Torus(obj->big_radius, obj->small_radius, theta_samples_menu, obj->phi_samples, torus_shader, obj->transform, obj->name);
        objects.erase(std::remove(objects.begin(), objects.end(), obj), objects.end());
        delete obj;
        selected_objects.erase(obj);
        add_torus(ptorus, false, true);
    }

    if (ImGui::SliderInt("phi", &phi_samples_menu, 3, 100) ) {
        auto ptorus = new Torus(obj->big_radius, obj->small_radius, obj->theta_samples, phi_samples_menu, torus_shader, obj->transform, obj->name);
        objects.erase(std::remove(objects.begin(), objects.end(), obj), objects.end());
        delete obj;
        selected_objects.erase(obj);
        add_torus(ptorus, false, true);
    }

    ImGui::End();
}

bool is_torus_selected() {
    return (selected_objects.size() == 1) && ((*selected_objects.begin())->uid == 1);
}

void render_gui() {
    if (selected_objects.size() == 1) {
        render_single_object_transform_menu();
    }

    if (is_torus_selected()) {
        render_torus_menu();
    }

    render_options_menu();
    render_objects_list_window();
    render_fps_counter();
}

unsigned int gridVAO, gridVBO;

void initializeGridBuffers() {
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(vec3), gridVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void render_grid() {
    glUseProgram(grid_shader_program);

    glUniformMatrix4fv(glGetUniformLocation(grid_shader_program, "projection"), 1, GL_FALSE, value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(grid_shader_program, "view"), 1, GL_FALSE, value_ptr(view));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertices.size());
    glBindVertexArray(0);
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
    glEnable(GL_STENCIL_TEST);

    initializeGridBuffers();

    lastTime = std::chrono::high_resolution_clock::now();

    grid_shader_program = shader_manager.shader_program({"grid"});
    cursor_shader = shader_manager.shader_program({"cursor"});
    torus_shader = shader_manager.shader_program({"torus"});
    point_shader = shader_manager.shader_program({"point"});

    objects.emplace_back(new Cursor(cursor_shader));

    objects.emplace_back(new Torus(1.0f, 0.1f, 25, 25, torus_shader));

    glEnable(GL_PROGRAM_POINT_SIZE);

    while (!glfwWindowShouldClose(window)) {
        processInput();
        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glViewport(0, 0, width, height);

        float ascpet_ratio = static_cast<float>(width) / static_cast<float>(height);
        projection = perspective(radians(fov),  ascpet_ratio, 0.1f, orbit_distance * 10.0f);

        camera_position = target_position + vec3(
            orbit_distance * cos(radians(orbit_yaw)) * cos(radians(orbit_pitch)),
            orbit_distance * sin(radians(orbit_pitch)),
            orbit_distance * sin(radians(orbit_yaw)) * cos(radians(orbit_pitch))
        );

        view = lookAt(camera_position, target_position, vec3(0.0f, 1.0f, 0.0f));

        render_grid();

        for (auto& object : objects) {
            vec3 color = vec3(0.8f, 0.6f, 0.2f);
            if (selected_objects.contains(object)) {
                color = vec3(1.0f, 0.8f, 0.3f);
            }
            object->draw(projection, view, color);
        }

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
