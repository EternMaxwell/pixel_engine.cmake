#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in int model_id;

layout(location = 0) out vec4 frag_color;

layout(std140, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
};

layout(std140, binding = 1) buffer ModelBuffer { mat4 model[]; };

void main() {
    mat4 model_matrix = model[model_id];
    gl_Position       = projection * view * model_matrix * vec4(pos, 0.0, 1.0);
    frag_color        = color;
}