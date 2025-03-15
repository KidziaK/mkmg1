#include <GLFW/glfw3.h>

extern int width, height;
extern double lastX, lastY;
extern bool leftMousePressed;
extern float orbit_yaw, orbit_pitch, mouseSensitivity;
extern float orbit_distance, zoomSensitivity;
extern GLFWwindow* window;

inline void framebuffer_size_callback(GLFWwindow* window_, int width_, int height_) {
    width = width_;
    height = height_;
    glViewport(0, 0, width_, height_);
}

inline void processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (leftMousePressed) {
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
}

inline void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        leftMousePressed = action == GLFW_PRESS;
        if (leftMousePressed) {
            glfwGetCursorPos(window, &lastX, &lastY);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

inline void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    orbit_distance -= (float)yoffset * zoomSensitivity;
    if (orbit_distance < 1.0f)
        orbit_distance = 1.0f;
}
