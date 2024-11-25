#version 450

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 position;
layout(location = 2) in int model_id;

layout(location = 0) out vec2 vert_position;
layout(location = 1) out vec4 geom_color;
layout(location = 2) out int geom_model_id;

void main() {
    vert_position = position;
    geom_color    = color;
    geom_model_id = model_id;
}