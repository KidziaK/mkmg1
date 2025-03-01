#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include "vec4.h"
#include "mat4.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void renderEllipsoid(unsigned char* buffer, int width, int height, const mat4& M, const mat4& D, float m, int chunk_size);
mat4 createTransformationMatrix(float scale, float rotX, float rotY, float rotZ, float transX, float transY, float transZ);

bool firstMouse = true;
float lastX = 400, lastY = 300;
float yaw = 45.0f, pitch = 45.0f;
float scale = 1.0f;
float transX = 0.0f, transY = 0.0f;
int chunk_size = 8;
int effective_chunk_size = 8;
bool mousePressed = false;
bool rotating = false;

static constexpr int windowWidth = 1200, windowHeight = 800;

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Ellipsoid Visualizer", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float ellipsoidA = 5.0f;
    float ellipsoidB = 1.0f;
    float ellipsoidC = 5.0f;
    float intensity = 1.0f;

    std::vector<unsigned char> frameBuffer(windowWidth * windowHeight * 3, 0);

    mat4 D = mat4::diag(ellipsoidA, ellipsoidB, ellipsoidC, -1.0f);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Parameters");
        ImGui::SliderFloat("a", &ellipsoidA, 1.0f, 10.0f);
        ImGui::SliderFloat("b", &ellipsoidB, 1.0f, 10.0f);
        ImGui::SliderFloat("c", &ellipsoidC, 1.0f, 10.0f);
        ImGui::SliderFloat("m", &intensity, 0.05f, 5.0f);
        if (ImGui::SliderInt("s", &chunk_size, 1, 8)) {
            effective_chunk_size = chunk_size;
        };
        ImGui::Text("(%.1f FPS)", ImGui::GetIO().Framerate);
        ImGui::End();

        D(0, 0) = ellipsoidA;
        D(1, 1) = ellipsoidB;
        D(2, 2) = ellipsoidC;

        mat4 transformMatrix = createTransformationMatrix(scale, pitch, yaw, 0.0f, transX, transY, 0.0f);
        renderEllipsoid(frameBuffer.data(), windowWidth, windowHeight, transformMatrix, D, intensity, effective_chunk_size);
        effective_chunk_size = std::max(1, effective_chunk_size / 2);

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
        ImGui::Begin("Rendered Result", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        const auto& imgui_texture = reinterpret_cast<ImTextureID>(reinterpret_cast<void *>(static_cast<intptr_t>(textureID)));
        ImGui::Image(imgui_texture, ImVec2(windowWidth, windowHeight));
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        effective_chunk_size = chunk_size;
        yaw += xoffset;
        pitch += yoffset;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        effective_chunk_size = chunk_size;
        transX += xoffset / windowWidth;
        transY += yoffset / windowHeight;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    effective_chunk_size = chunk_size;
    scale += yoffset * 0.1f;
    if (scale < 0.1f) scale = 0.1f;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void renderEllipsoid(unsigned char* buffer, int width, int height, const mat4& M, const mat4& D, float m, int chunk_size) {
    mat4 M_inv = M.inv();
    mat4 D_M = M_inv.t() * D * M_inv;

    #pragma omp parallel for
    for (int j = 0; j < height; j += chunk_size) {
        #pragma omp parallel for
        for (int i = 0; i < width; i += chunk_size) {
            unsigned char red = 25, green = 25, blue = 25;
            const float x = (2.0f * static_cast<float>(i)) / static_cast<float>(width) - 1.0f;
            const float y = 1.0f - (2.0f * static_cast<float>(j)) / static_cast<float>(height);

            vec4 rayOrigin(x, y, 5.0f, 1.0f);
            vec4 rayDirection(0.0f, 0.0f, -1.0f, 0.0f);

            float oDo = dot(rayOrigin,D_M * rayOrigin);
            float vDo = dot(rayDirection, D_M * rayOrigin);
            float oDv = dot(rayOrigin, D_M * rayDirection);
            float vDv = dot(rayDirection, D_M * rayDirection);

            const float a = vDv;
            const float b = vDo + oDv;
            const float c = oDo;

            const float discriminant = b * b - 4 * a * c;

            if (discriminant >= 0) {
                const float t = (-b - sqrtf(discriminant)) / (2 * a);
                vec4 hitPoint = rayOrigin + t * rayDirection;

                vec4 normal = (D_M + D_M.t()) * hitPoint;
                normal = normal.normalize();

                float illuminance = powf(dot(-rayDirection, normal), m);
                illuminance = fminf(illuminance, 1.0f);

                red = static_cast<char>(illuminance * 255.0f);
                green = static_cast<char>(illuminance * 225.0f);
                blue = 0;
            }

            for (int ki = 0; ki < chunk_size; ++ki) {
                for (int kj = 0; kj < chunk_size; ++kj) {
                    int i_ = i + ki;
                    int j_ = j + kj;
                    const int index = (j_ * width + i_) * 3;
                    buffer[index] = red;
                    buffer[index + 1] = green;
                    buffer[index + 2] = blue;
                }
            }
        }
    }
}

mat4 createTransformationMatrix(float scale, float rotX, float rotY, float rotZ, float transX, float transY, float transZ) {
    mat4 result = {};

    float radX = rotX * M_PI / 180.0f;
    float radY = rotY * M_PI / 180.0f;
    float radZ = rotZ * M_PI / 180.0f;

    // Rotation around X-axis
    mat4 rotMatX = {};
    rotMatX(0, 0) = 1.0f;
    rotMatX(1, 1) = cosf(radX);
    rotMatX(1, 2) = -sinf(radX);
    rotMatX(2, 1) = sinf(radX);
    rotMatX(2, 2) = cosf(radX);
    rotMatX(3, 3) = 1.0f;

    // Rotation around Y-axis
    mat4 rotMatY = {};
    rotMatY(0, 0) = cosf(radY);
    rotMatY(0, 2) = sinf(radY);
    rotMatY(1, 1) = 1.0f;
    rotMatY(2, 0) = -sinf(radY);
    rotMatY(2, 2) = cosf(radY);
    rotMatY(3, 3) = 1.0f;

    // Rotation around Z-axis
    mat4 rotMatZ = {};
    rotMatZ(0, 0) = cosf(radZ);
    rotMatZ(0, 1) = -sinf(radZ);
    rotMatZ(1, 0) = sinf(radZ);
    rotMatZ(1, 1) = cosf(radZ);
    rotMatZ(2, 2) = 1.0f;
    rotMatZ(3, 3) = 1.0f;

    // Scale
    mat4 scaleMatrix = {};
    scaleMatrix(0, 0) = scale;
    scaleMatrix(1, 1) = scale;
    scaleMatrix(2, 2) = scale;
    scaleMatrix(3, 3) = 1.0f;

    // Translation
    mat4 transMatrix = {};
    transMatrix(0, 0) = 1.0f;
    transMatrix(1, 1) = 1.0f;
    transMatrix(2, 2) = 1.0f;
    transMatrix(3, 3) = 1.0f;
    transMatrix(0, 3) = transX;
    transMatrix(1, 3) = transY;
    transMatrix(2, 3) = transZ;

    result = transMatrix * rotMatZ * rotMatY * rotMatX * scaleMatrix;

    return result;
}





