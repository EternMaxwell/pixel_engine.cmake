#version 450 core

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

layout(location = 0) uniform sampler2D tex;

void main() { fragColor = texture(tex, texCoord) * color; }