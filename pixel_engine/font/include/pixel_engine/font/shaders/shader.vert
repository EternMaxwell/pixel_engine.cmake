#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 color;

layout(push_constant) uniform Model { mat4 model; }
model;

layout(binding = 0, std140) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
};

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

void main() {
    gl_Position = proj * view * model.model * vec4(position, 1.0);
    fragTexCoord = texCoord;
    fragColor = color;
}