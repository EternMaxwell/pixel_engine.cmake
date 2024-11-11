#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 color;
layout(location = 3) in int layer;
layout(location = 4) in int texture_index;
layout(location = 5) in int model_index;

layout(set = 0, binding = 0, std140) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
};

layout(set = 0, binding = 1, std430) buffer Models {
    mat4 models[];
};

layout(location = 0) out vec3 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out int tex_id;

void main() {
    gl_Position = proj * view * models[model_index] * vec4(position, 1.0);
    fragTexCoord = vec3(texCoord, float(layer));
    fragColor = color;
    tex_id = texture_index;
}