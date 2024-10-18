#version 450

layout(location = 0) in vec2 tex_coords;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 frag_color;

layout(binding = 1) uniform sampler2D tex;

void main() { frag_color = texture(tex, tex_coords) * color; }