#version 460 core

in vec3 worldPos;
out vec4 FragColor;

void main() {
    float lineWidth = 0.01;

    vec4 background_color = vec4(0.1, 0.1, 0.1, 1.0);
    vec4 grid_lines_color = vec4(0.5, 0.5, 0.5, 1.0);
    vec4 z_axis_color = vec4(0.0, 0.0, 1.0, 1.0);
    vec4 x_axis_color = vec4(1.0, 0.0, 0.0, 1.0);

    if (abs(worldPos.y) < 1e-5f) {
        if (abs(worldPos.x) < 1e-5f) {
            FragColor = z_axis_color;
        } else if (abs(worldPos.z) < 1e-5f) {
            FragColor = x_axis_color;
        } else if (abs(worldPos.x - round(worldPos.x)) < lineWidth || abs(worldPos.z - round(worldPos.z)) < lineWidth) {
            FragColor =  grid_lines_color;
        } else {
            FragColor = background_color;
        }
    } else {
        discard;
    }
}
