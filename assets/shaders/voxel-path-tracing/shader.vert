#version 450 core

layout(location = 0) in vec2 in_pos;

layout(location = 0) out vec2 out_pos;

void main() {
    out_pos = in_pos;
    gl_Position = vec4(in_pos, 0.0, 1.0);
}
