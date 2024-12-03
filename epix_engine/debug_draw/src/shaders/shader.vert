#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in uint model_id;

layout(location = 0) out vec4 color;

layout(std140, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
};

layout(std140, binding = 1) buffer ModelBuffer { mat4 models[]; };

void main() {
    gl_Position = projection * view * models[model_id] * vec4(in_position, 1.0);
    color       = in_color;
}