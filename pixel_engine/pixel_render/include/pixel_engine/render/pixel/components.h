#pragma once

#include <pixel_engine/app.h>
#include <pixel_engine/render_vk.h>

#include <glm/glm.hpp>

namespace pixel_engine::render::pixel {
namespace components {
using namespace pixel_engine::prelude;

struct PixelBlockVertex {
    glm::vec4 color;
    int model_index;
};

struct PixelVertex {
    glm::vec4 color;
    glm::vec2 pos;
    int model_index;
};

struct PixelUniformBuffer {
    glm::mat4 view;
    glm::mat4 proj;
};

struct PixelBlockData {
    glm::mat4 model;
    glm::uvec2 size;
    uint32_t offset;
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
struct PixelBlockRenderer {
    RenderPass render_pass;
    Pipeline graphics_pipeline;
    PipelineLayout pipeline_layout;
    Buffer vertex_buffer;
    Buffer vertex_staging_buffer;
    Buffer uniform_buffer;
    Buffer block_model_buffer;
    DescriptorSetLayout descriptor_set_layout;
    DescriptorPool descriptor_pool;
    DescriptorSet descriptor_set;
    Fence fence;
    CommandBuffer command_buffer;
    Framebuffer framebuffer;

    struct Context {
        Device* device;
        Queue* queue;
        vk::Extent2D extent;
        uint32_t vertex_offset = 0;
        int block_model_offset = 0;
        PixelBlockData* block_model_data;
        PixelBlockVertex* vertex_data;

        Context(Device& device, Queue& queue);
    };
    std::optional<Context> context;

    void begin(
        Device& device,
        Swapchain& swapchain,
        Queue& queue,
        const PixelUniformBuffer& pixel_uniform
    );
    void begin(
        Device& device,
        Queue& queue,
        ImageView& render_target,
        vk::Extent2D extent,
        const PixelUniformBuffer& pixel_uniform
    );
    void begin(
        Device& device,
        Queue& queue,
        Framebuffer& framebuffer,
        vk::Extent2D extent,
        const PixelUniformBuffer& pixel_uniform
    );
    void draw(const PixelBlock& block, const BlockPos2d& pos2d);
    void end();

   private:
    void set_block_data(const PixelBlock& block, const BlockPos2d& pos2d);
    void reset_cmd();
    void flush();
};

struct PixelRenderer {
    RenderPass render_pass;
    Pipeline graphics_pipeline;
    PipelineLayout pipeline_layout;
    Buffer vertex_buffer;
    Buffer uniform_buffer;
    Buffer block_model_buffer;
    DescriptorSetLayout descriptor_set_layout;
    DescriptorPool descriptor_pool;
    DescriptorSet descriptor_set;
    Fence fence;
    CommandBuffer command_buffer;
    Framebuffer framebuffer;

    struct Context {
        Device* device;
        Queue* queue;
        vk::Extent2D extent;
        uint32_t vertex_offset = 0;
        int model_offset       = 0;
        glm::mat4* model_data;
        PixelVertex* vertex_data;

        Context(Device& device, Queue& queue);
    };

    std::optional<Context> context;

    void begin(
        Device& device,
        Swapchain& swapchain,
        Queue& queue,
        const PixelUniformBuffer& pixel_uniform
    );
    void begin(
        Device& device,
        Queue& queue,
        ImageView& render_target,
        vk::Extent2D extent,
        const PixelUniformBuffer& pixel_uniform
    );
    void begin(
        Device& device,
        Queue& queue,
        Framebuffer& framebuffer,
        vk::Extent2D extent,
        const PixelUniformBuffer& pixel_uniform
    );
    void draw(const glm::vec4& color, const glm::vec2& pos);
    void end();
    void set_model(const glm::mat4& model);
    void set_model(
        const glm::vec2& pos, const glm::vec2& scale, float rotation
    );

   private:
    void reset_cmd();
    void flush();
};
}  // namespace components
}  // namespace pixel_engine::render::pixel