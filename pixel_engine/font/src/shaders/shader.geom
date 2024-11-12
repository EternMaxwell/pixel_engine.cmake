#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 6) out;

layout(std140, set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
};

struct ModelData {
    mat4 model;
    vec4 color;
    int texture_index;
};

layout(std140, set = 0, binding = 1) buffer Models { ModelData models[]; };

layout(location = 0) in vec2 position[];
layout(location = 1) in vec2 tex_coord_lb[];
layout(location = 2) in vec2 tex_coord_rt[];
layout(location = 3) in vec2 size[];
layout(location = 4) in flat int layer_id[];
layout(location = 5) in flat int model_id[];

layout(location = 0) out vec3 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out int tex_id;

void main() {
    int mod_id    = model_id[0];
    mat4 mvp      = proj * view * models[mod_id].model;
    vec2 geom_pos = vec2(position[0].x, position[0].y);
    float dx      = size[0].x;
    float dy      = size[0].y;

    gl_Position  = mvp * vec4(geom_pos, 0, 1);
    fragTexCoord = vec3(tex_coord_lb[0], layer_id[0]);
    fragColor    = models[mod_id].color;
    tex_id       = models[mod_id].texture_index;
    EmitVertex();
    gl_Position  = mvp * vec4(geom_pos.x + dx, geom_pos.y, 0, 1);
    fragTexCoord = vec3(tex_coord_rt[0].x, tex_coord_lb[0].y, layer_id[0]);
    fragColor    = models[mod_id].color;
    tex_id       = models[mod_id].texture_index;
    EmitVertex();
    gl_Position  = mvp * vec4(geom_pos.x + dx, geom_pos.y + dy, 0, 1);
    fragTexCoord = vec3(tex_coord_rt[0], layer_id[0]);
    fragColor    = models[mod_id].color;
    tex_id       = models[mod_id].texture_index;
    EmitVertex();
    EndPrimitive();
    gl_Position  = mvp * vec4(geom_pos.x, geom_pos.y, 0, 1);
    fragTexCoord = vec3(tex_coord_lb[0], layer_id[0]);
    fragColor    = models[mod_id].color;
    tex_id       = models[mod_id].texture_index;
    EmitVertex();
    gl_Position  = mvp * vec4(geom_pos.x + dx, geom_pos.y + dy, 0, 1);
    fragTexCoord = vec3(tex_coord_rt[0], layer_id[0]);
    fragColor    = models[mod_id].color;
    tex_id       = models[mod_id].texture_index;
    EmitVertex();
    gl_Position  = mvp * vec4(geom_pos.x, geom_pos.y + dy, 0, 1);
    fragTexCoord = vec3(tex_coord_lb[0].x, tex_coord_rt[0].y, layer_id[0]);
    fragColor    = models[mod_id].color;
    tex_id       = models[mod_id].texture_index;
    EmitVertex();
    EndPrimitive();
}