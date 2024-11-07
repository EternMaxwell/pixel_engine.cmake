#version 450

layout(location = 0) in vec4 color;
layout(location = 1) in int model_id;

layout(location = 0) out int geom_vertex_id;
layout(location = 1) out vec4 geom_color;
layout(location = 2) out int geom_model_id;

void main() {
    geom_vertex_id = gl_VertexIndex;
    geom_color    = color;
    geom_model_id = model_id;
}