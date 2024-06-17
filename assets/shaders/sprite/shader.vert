#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexCoord;

layout(binding = 0, std140) uniform UniformBufferObject {
    mat4 proj;
    mat4 view;
    mat4 model;
};

void main() {
    mat4 mvp = proj * view * model;
    gl_Position = mvp * vec4(position, 1.0);
    outColor = color;
    outTexCoord = texCoord;
}