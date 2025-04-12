#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform bool u_selected;

out vec3 color;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    if (u_selected) {
        color = vec3(1.0f, 0.8f, 0.3f);
    } else {
        color = vec3(0.8f, 0.6f, 0.2f);
    }

}
