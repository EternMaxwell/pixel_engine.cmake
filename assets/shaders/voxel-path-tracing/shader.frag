#version 450 core

layout(location = 0) in vec2 in_pos;  // pos on the screen (-1, 1).

layout(location = 0) out vec4 out_color;  // color of the pixel.

layout(std140, binding = 0) uniform Camera {
};