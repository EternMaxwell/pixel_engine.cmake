#version 450

layout(location = 0) in vec3 tex_coords;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 frag_color;

layout(binding = 1) uniform usampler2DArray tex;

void main() {
    float v = texture(tex, tex_coords).r / 255.0;
    frag_color = v * color;
}