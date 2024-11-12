#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 texCoords;
layout(location = 2) in vec2 size;
layout(location = 3) in int layer;
layout(location = 4) in int model_index;

layout(location = 0) out vec2 pos;
layout(location = 1) out vec2 tex_coord_lb;
layout(location = 2) out vec2 tex_coord_rt;
layout(location = 3) out vec2 size_o;
layout(location = 4) out int layer_id;
layout(location = 5) out int model_id;

void main() {
    pos          = position;
    tex_coord_lb = texCoords.xy;
    tex_coord_rt = texCoords.zw;
    size_o       = size;
    layer_id     = layer;
    model_id     = model_index;
}