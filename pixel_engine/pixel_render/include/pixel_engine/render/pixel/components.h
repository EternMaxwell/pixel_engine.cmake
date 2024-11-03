#pragma once

#include <pixel_engine/app.h>
#include <pixel_engine/render_vk.h>

#include <glm/glm.hpp>

namespace pixel_engine::render::pixel {
namespace components {
using namespace pixel_engine::prelude;

struct PixelVertex {
    glm::vec2 pos;
    glm::vec4 color;
    int model_index;
};

struct PixelUniformBuffer {
    glm::mat4 view;
    glm::mat4 proj;
};

struct PixelBlock {
    static PixelBlock create(glm::uvec2 size);

    glm::vec4& operator[](glm::uvec2 pos);
    const glm::vec4& operator[](glm::uvec2 pos) const;

    std::vector<glm::vec4> pixels;
    glm::uvec2 size;
};

struct BlockPos2d {
    glm::vec3 pos   = glm::vec3(0.0f);
    float rotation  = 0.0f;
    glm::vec2 scale = glm::vec2(1.0f);
};

using namespace pixel_engine::render_vk::components;
struct PixelRenderer {
    RenderPass render_pass;
    Pipeline graphics_pipeline;
    PipelineLayout pipeline_layout;
    Buffer vertex_buffer;
    Buffer index_buffer;
    Buffer uniform_buffer;
    Buffer block_model_buffer;
    DescriptorSetLayout descriptor_set_layout;
    DescriptorPool descriptor_pool;
    DescriptorSet descriptor_set;
};
}  // namespace components
}  // namespace pixel_engine::render::pixel