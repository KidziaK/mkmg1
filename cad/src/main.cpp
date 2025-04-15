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
#include <map>
#include <random>

using namespace myglm;

// window
GLFWwindow* window = nullptr;
int width = 1920, height = 1080;

// mouse
double lastX = width / 2.0;
double lastY = height / 2.0;
bool leftMousePressed = false;
bool rightMousePressed = false;
bool middleMousePressed = false;
bool leftMouseDown = false;
double boxStartX = 0;
double boxStartY = 0;
bool shiftDown = false;
bool leftCtrlDown = false;
bool qKeyPressed = false;
bool cKeyPressed = false;

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
bool showTransformCursorMenu = false;
bool showTransformMeanMenu = false;

// FPS counter
std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
int frameCount = 0;
float fps = 0.0f;

// static objects
static constexpr auto gridVertices = generateGridVertices(gridSize);

// dynamic objects
std::vector<Object*> objects = {};
std::unordered_set<Object*> selected_objects;
Cursor* center_point;

// transform window
float transform_window_trans[3] = {0, 0, 0};
float transform_window_rot[3] = {0, 0, 0};
float transform_window_scale[3] = {1, 1, 1};
Transform cursor_relative_transform = Transform::identity();
Transform center_point_relative_transform = Transform::identity();
float cursor_relative_transform_window_rot[3] = {0, 0, 0};

// torus
float big_radius_menu;
float small_radius_menu;
int theta_samples_menu;
int phi_samples_menu;

// other
mat4 cursor_relative_mat4 = mat4(1.0f);
mat4 center_point_relative_mat4 = mat4(1.0f);

void framebuffer_size_callback(GLFWwindow* window_, int width_, int height_) {
    width = width_;
    height = height_;
    glViewport(0, 0, width_, height_);
}

void add_point() {
    auto ppoint = new Point(point_shader);
    objects.push_back(ppoint);
    for (auto& object : selected_objects) {
        if (object->uid == 4) {
            C0Bezier* bezier = dynamic_cast<C0Bezier*>(object);
            bezier->control_points.push_back(ppoint);
            bezier->control_polygon->points.push_back(ppoint);
        }
    }
    ppoint->transform = objects[0]->transform;
}

void processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        shiftDown = true;
    } else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
        shiftDown = false;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        leftCtrlDown = true;
    } else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE) {
        leftCtrlDown = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && !qKeyPressed) {
        add_point();
        qKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_RELEASE) {
        qKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cKeyPressed) {
        for (auto& object : objects) {
            if (object->uid == 4) {
                C0Bezier* bezier = dynamic_cast<C0Bezier*>(object);
                bezier->show_control_polygon = !bezier->show_control_polygon;
            }
        }
        cKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        cKeyPressed = false;
    }

    if (rightMousePressed && !ImGui::GetIO().WantCaptureMouse) {
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
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        rightMousePressed = true;

        glfwGetCursorPos(window, &lastX, &lastY);
        if (!ImGui::GetIO().WantCaptureMouse) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        leftMousePressed = true;
        leftMouseDown = true;

        glfwGetCursorPos(window, &lastX, &lastY);
        boxStartX = lastX;
        boxStartY = lastY;
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        middleMousePressed = true;
        glfwGetCursorPos(window, &lastX, &lastY);
        if (!ImGui::GetIO().WantCaptureMouse) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        leftMousePressed = false;
        leftMouseDown = false;

        if (!ImGui::GetIO().WantCaptureMouse) {
            glfwGetCursorPos(window, &lastX, &lastY);

            if (!shiftDown) {
                selected_objects.clear();
            }


            int xMin = std::min(boxStartX, lastX);
            int xMax = std::max(boxStartX, lastX);
            int yMin = std::min(boxStartY, lastY);
            int yMax = std::max(boxStartY, lastY);

            static constexpr unsigned int radius = 3;

            int dx = xMax - xMin + radius;
            int dy = yMax - yMin + radius;

            size_t bufferSize = dx * dy * sizeof(GLubyte) * 2;
            std::vector<GLubyte> pixels(bufferSize);

            glReadPixels(xMin, height - yMax, dx, dy, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, pixels.data());

            for (auto pixel : pixels) {
                size_t index = pixel;
                if (index > 0) {
                    selected_objects.insert(objects[index - 1]);
                }
            }
        }
    }
    else {
        leftMousePressed = false;
        rightMousePressed = false;
        middleMousePressed = false;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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
        add_point();
    }

    if (ImGui::Button("Polyline")) {
        std::vector<Point*> points;
        for (auto& object : selected_objects) {
            if (object->uid == 2) {
                points.push_back(dynamic_cast<Point*>(object));
            }
        }
        if (points.size() >= 2) {
            auto ppolyline = new PolyLine(point_shader, points);
            objects.push_back(ppolyline);
        }
    }

    if (ImGui::Button("C0 Bezier")) {
        std::vector<Point*> points;
        for (auto& object : selected_objects) {
            if (object->uid == 2) {
                points.push_back(dynamic_cast<Point*>(object));
            }
        }

        auto pc0bezier = new C0Bezier(point_shader, points);
        objects.push_back(pc0bezier);
    }

    ImGui::End();
}

void render_single_object_transform_menu() {
    ImGui::Begin("Local Transform", &showTransformMenu);
    auto& selected_obj = *selected_objects.begin();

    transform_window_trans[0] = selected_obj->transform.translation.x;
    transform_window_trans[1] = selected_obj->transform.translation.y;
    transform_window_trans[2] = selected_obj->transform.translation.z;

    transform_window_scale[0] = selected_obj->transform.s.x;
    transform_window_scale[1] = selected_obj->transform.s.y;
    transform_window_scale[2] = selected_obj->transform.s.z;

    vec3 euler_angles = selected_obj->transform.rotation;

    transform_window_rot[0] = degrees(euler_angles.x);
    transform_window_rot[1] = degrees(euler_angles.y);
    transform_window_rot[2] = degrees(euler_angles.z);

    if (ImGui::SliderFloat3("translation", transform_window_trans, -5.0f, 5.0f) ) {
        selected_obj->transform.translation.x = transform_window_trans[0];
        selected_obj->transform.translation.y = transform_window_trans[1];
        selected_obj->transform.translation.z = transform_window_trans[2];
    }
    if (ImGui::SliderFloat3("rotation", transform_window_rot, -360.0f, 360.0f) ) {
        float rad_x = radians(transform_window_rot[0]);
        float rad_y = radians(transform_window_rot[1]);
        float rad_z = radians(transform_window_rot[2]);
        selected_obj->transform.rotation = {rad_x, rad_y, rad_z};
    }
    if (ImGui::SliderFloat3("scale", transform_window_scale, 0.1f, 5.0f) ) {
        selected_obj->transform.s.x = transform_window_scale[0];
        selected_obj->transform.s.y = transform_window_scale[1];
        selected_obj->transform.s.z = transform_window_scale[2];
    }
    ImGui::End();
}

void render_transform_around_cursor_menu() {
    ImGui::Begin("Transform with respect to cursor", &showTransformCursorMenu);

    if (ImGui::SliderFloat3("rotation", value_ptr(cursor_relative_transform.rotation), -360.0f, 360.0f) ) {

    }
    if (ImGui::SliderFloat("scale", &cursor_relative_transform.s.x, 0.1f, 5.0f) ) {
        cursor_relative_transform.s.y = cursor_relative_transform.s.x;
        cursor_relative_transform.s.z = cursor_relative_transform.s.x;
    }

    if (ImGui::Button("apply")) {
        for (auto& object : objects) {
            if (object->uid > 0 && selected_objects.contains(object)) {
                object->transform = Transform::from_mat4(object->transform.to_mat4() * cursor_relative_mat4);
            }
        }

        cursor_relative_transform = Transform::identity();
    }

    ImGui::End();
}

void render_transform_around_center_menu() {
    ImGui::Begin("Transform with respect to center point", &showTransformCursorMenu);

    if (ImGui::SliderFloat3("translation", value_ptr(center_point_relative_transform.translation), -5.0f, 5.0f) ) {

    }
    if (ImGui::SliderFloat3("rotation", value_ptr(center_point_relative_transform.rotation), -360.0f, 360.0f) ) {

    }
    if (ImGui::SliderFloat("scale", &center_point_relative_transform.s.x, 0.1f, 5.0f) ) {
        center_point_relative_transform.s.y = center_point_relative_transform.s.x;
        center_point_relative_transform.s.z = center_point_relative_transform.s.x;
    }

    if (ImGui::Button("apply")) {
        for (auto& object : objects) {
            if (object->uid > 0 && selected_objects.contains(object)) {
                object->transform = Transform::from_mat4(object->transform.to_mat4() * center_point_relative_mat4);
            }
        }

        center_point_relative_transform = Transform::identity();
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

        bool style_selected = selected_objects.contains(obj);

        if (style_selected) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 0.8f, 0.2f));
        }

        if (ImGui::InputText("", buffer, IM_ARRAYSIZE(buffer))) {
            obj->name = buffer;
        }

        if (style_selected) {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemClicked()) {
            if (!shiftDown) {
                selected_objects.clear();
            }

            selected_objects.insert(objects[i]);

            if (leftCtrlDown && objects[i]->uid == 2) {
                Point* point = dynamic_cast<Point*>(objects[i]);
                for (auto& object : selected_objects) {
                    if (object->uid == 4) {
                        C0Bezier* bezier = dynamic_cast<C0Bezier*>(object);
                        bezier->control_points.push_back(point);
                        bezier->control_polygon->points.push_back(point);
                    }
                }
            }
        }

        if (item_name != "cursor") {
            ImGui::SameLine();
            if (ImGui::Button("X")) {

                if (obj->uid == 2) {
                    for (auto& object : objects) {
                        if (object->uid == 4) {
                            C0Bezier* bezier = dynamic_cast<C0Bezier*>(object);
                            int idx = -1;
                            for (int i = 0; i < bezier->control_points.size(); i++) {
                                auto point = bezier->control_points[i];
                                if (point == obj) {
                                    idx = i;
                                    break;
                                }
                            }
                            if (idx >= 0) {
                                bezier->control_points.erase(bezier->control_points.begin() + idx);
                                bezier->control_polygon->points.erase(bezier->control_polygon->points.begin() + idx);
                            }
                        }
                    }
                }

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

    if (!selected_objects.empty()) {
        render_transform_around_cursor_menu();
    } else {
        cursor_relative_transform = Transform::identity();
    }

    if (selected_objects.size() >= 2) {
        render_transform_around_center_menu();
    } else {
        center_point_relative_transform = Transform::identity();
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
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

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

    center_point = new Cursor(cursor_shader);

    glEnable(GL_PROGRAM_POINT_SIZE);

    while (!glfwWindowShouldClose(window)) {
        processInput();
        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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

        vec3 cursor_translation = objects[0]->transform.translation;
        cursor_relative_mat4 = trans_mat(-cursor_translation) * cursor_relative_transform.to_mat4() * trans_mat(cursor_translation);

        vec3 center_point_translation = center_point->transform.translation;
        center_point_relative_mat4 = trans_mat(-center_point_translation) * center_point_relative_transform.to_mat4() * trans_mat(center_point_translation);

        mat4 relative_transform = cursor_relative_mat4 * center_point_relative_mat4;

        for (int i = 0; i < objects.size(); i++) {
            auto& object = objects[i];

            glStencilFunc(GL_ALWAYS, i + 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

            object->update(relative_transform, selected_objects, projection, view, width, height);
            mat4 global_transform = selected_objects.contains(object) ? relative_transform : mat4(1.0f);
            object->draw(projection, view, selected_objects.contains(object), global_transform);
        }

        if (!selected_objects.empty()) {
            center_point->transform = Transform::identity();
            center_point->transform.s = vec3(0.5f, 0.5f, 0.5f);

            float counter = 0.0f;

            for (auto& object : selected_objects) {
                if (object->uid < 3) {
                    center_point->transform.translation += object->transform.translation;
                    counter++;
                }
            }

            if (counter > 0) {
                center_point->transform.translation /= counter;
                center_point->draw(projection, view, false, center_point_relative_mat4);
            }
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
