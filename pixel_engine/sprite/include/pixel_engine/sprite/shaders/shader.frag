#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 color;
layout(location = 2) flat in int textureIndex;

layout(location = 0) out vec4 fragColor;

layout(binding = 2) uniform sampler2D textures[];

void main() {
    fragColor = color * texture(textures[textureIndex], texCoord);
}