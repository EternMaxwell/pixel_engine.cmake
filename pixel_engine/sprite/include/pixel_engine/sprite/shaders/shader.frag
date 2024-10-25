#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 color;
layout(location = 2) flat in int textureIndex;
layout(location = 3) flat in int samplerIndex;

layout(location = 0) out vec4 fragColor;

layout(binding = 2) uniform texture2D textures[65536];
layout(binding = 3) uniform sampler samplers[256];

void main() {
    fragColor =
        color *
        texture(
            sampler2D(textures[textureIndex], samplers[samplerIndex]), texCoord
        );
}