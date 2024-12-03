#pragma once

#include <pixel_engine/render_vk.h>

#include <glm/glm.hpp>

namespace pixel_engine::render::debug::vulkan::components {
using namespace pixel_engine::prelude;
using namespace pixel_engine::render_vk::components;
struct DebugVertex {
    glm::vec3 pos;
    glm::vec4 color;
    uint32_t model_index;
};
struct LineDrawer {
    const size_t max_vertex_count;
    const size_t max_model_count;
    RenderPass render_pass;
    Pipeline pipeline;
    PipelineLayout pipeline_layout;
    DescriptorSetLayout descriptor_set_layout;
    DescriptorPool descriptor_pool;
    DescriptorSet descriptor_set;
    Buffer vertex_buffer;
    Buffer uniform_buffer;
    Buffer model_buffer;
    Fence fence;
    CommandBuffer command_buffer;
    Framebuffer framebuffer;

    struct Context {
        Device* device;
        Queue* queue;
        vk::Extent2D extent;
        uint32_t vertex_count = 0;
        uint32_t model_count  = 0;
        DebugVertex* mapped_vertex_buffer;
        glm::mat4* mapped_model_buffer;
        Image color_image;

        Context(Device& device, Queue& queue)
            : device(&device), queue(&queue) {}
    };

    std::optional<Context> context;

    EPIX_API void begin(Device& device, Queue& queue, Swapchain& swapchain);
    EPIX_API void flush();
    EPIX_API void end();
    EPIX_API void setModel(const glm::mat4& model);
    EPIX_API void reset_cmd();
    EPIX_API void drawLine(
        const glm::vec3& start, const glm::vec3& end, const glm::vec4& color
    );
};
struct TriangleDrawer {
    const size_t max_vertex_count;
    const size_t max_model_count;
    RenderPass render_pass;
    Pipeline pipeline;
    PipelineLayout pipeline_layout;
    DescriptorSetLayout descriptor_set_layout;
    DescriptorPool descriptor_pool;
    DescriptorSet descriptor_set;
    Buffer vertex_buffer;
    Buffer uniform_buffer;
    Buffer model_buffer;
    Fence fence;
    CommandBuffer command_buffer;
    Framebuffer framebuffer;

    struct Context {
        Device* device;
        Queue* queue;
        vk::Extent2D extent;
        uint32_t vertex_count = 0;
        uint32_t model_count  = 0;
        DebugVertex* mapped_vertex_buffer;
        glm::mat4* mapped_model_buffer;
        Image color_image;

        Context(Device& device, Queue& queue)
            : device(&device), queue(&queue) {}
    };

    std::optional<Context> context;

    EPIX_API void begin(Device& device, Queue& queue, Swapchain& swapchain);
    EPIX_API void flush();
    EPIX_API void end();
    EPIX_API void setModel(const glm::mat4& model);
    EPIX_API void reset_cmd();
    EPIX_API void drawTriangle(
        const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
        const glm::vec4& color
    );
};
struct PointDrawer {
    const size_t max_vertex_count;
    const size_t max_model_count;
    RenderPass render_pass;
    Pipeline pipeline;
    PipelineLayout pipeline_layout;
    DescriptorSetLayout descriptor_set_layout;
    DescriptorPool descriptor_pool;
    DescriptorSet descriptor_set;
    Buffer vertex_buffer;
    Buffer uniform_buffer;
    Buffer model_buffer;
    Fence fence;
    CommandBuffer command_buffer;
    Framebuffer framebuffer;

    struct Context {
        Device* device;
        Queue* queue;
        vk::Extent2D extent;
        uint32_t vertex_count = 0;
        uint32_t model_count  = 0;
        DebugVertex* mapped_vertex_buffer;
        glm::mat4* mapped_model_buffer;
        Image color_image;

        Context(Device& device, Queue& queue)
            : device(&device), queue(&queue) {}
    };

    std::optional<Context> context;

    EPIX_API void begin(Device& device, Queue& queue, Swapchain& swapchain);
    EPIX_API void flush();
    EPIX_API void end();
    EPIX_API void setModel(const glm::mat4& model);
    EPIX_API void reset_cmd();
    EPIX_API void drawPoint(const glm::vec3& pos, const glm::vec4& color);
};
}  // namespace pixel_engine::render::debug::vulkan::components