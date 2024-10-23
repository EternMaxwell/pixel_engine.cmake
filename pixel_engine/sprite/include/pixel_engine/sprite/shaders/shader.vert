#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 color;
layout(location = 3) in int modelIndex;
layout(location = 4) in int textureIndex;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) flat out int fragTextureIndex;

layout(std140, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
};

layout(std430, binding = 1) buffer SpriteModelBuffer {
    mat4 model[];
};

void main() {
    mat4 modelMatrix = model[modelIndex];
    gl_Position = projection * view * modelMatrix * vec4(position, 1.0);
    fragTexCoord = texCoord;
    fragColor = color;
    fragTextureIndex = textureIndex;
}
