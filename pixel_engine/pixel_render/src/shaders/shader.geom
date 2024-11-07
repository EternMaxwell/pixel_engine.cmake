#version 450

layout(points) in;

layout(triangle_strip, max_vertices = 6) out;

layout(std140, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
};

struct BlockData {
    mat4 model;
    uvec2 size;
    uint offset;
};

layout(std140, binding = 1) buffer ModelBuffer { BlockData blocks[]; };

layout(location = 0) in int vertex_id[];
layout(location = 1) in vec4 geom_color[];
layout(location = 2) in int geom_model_id[];

layout(location = 0) out vec4 frag_color;

void main() {
    mat4 mvp = projection * view * blocks[geom_model_id[0]].model;
    vec2 geom_pos = vec2(
        (vertex_id[0] - blocks[geom_model_id[0]].offset) %
            blocks[geom_model_id[0]].size.x,
        (vertex_id[0] - blocks[geom_model_id[0]].offset) /
            blocks[geom_model_id[0]].size.x
    );

    gl_Position = mvp * vec4(geom_pos, 0, 1);
    frag_color  = geom_color[0];
    EmitVertex();
    gl_Position = mvp * vec4(geom_pos.x + 1, geom_pos.y, 0, 1);
    frag_color  = geom_color[0];
    EmitVertex();
    gl_Position = mvp * vec4(geom_pos.x + 1, geom_pos.y + 1, 0, 1);
    frag_color  = geom_color[0];
    EmitVertex();
    EndPrimitive();
    gl_Position = mvp * vec4(geom_pos.x, geom_pos.y, 0, 1);
    frag_color  = geom_color[0];
    EmitVertex();
    gl_Position = mvp * vec4(geom_pos.x + 1, geom_pos.y + 1, 0, 1);
    frag_color  = geom_color[0];
    EmitVertex();
    gl_Position = mvp * vec4(geom_pos.x, geom_pos.y + 1, 0, 1);
    frag_color  = geom_color[0];
    EmitVertex();
    EndPrimitive();
}
