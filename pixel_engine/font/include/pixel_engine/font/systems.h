#pragma once

#include <pixel_engine/render_vk.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include "components.h"
#include "resources.h"

namespace pixel_engine {
namespace font {
namespace systems {
using namespace prelude;
using namespace font::components;
using namespace render_vk::components;
using namespace font::resources;

static std::shared_ptr<spdlog::logger> logger =
    spdlog::default_logger()->clone("font");

struct TextVertex {
    float pos[3];
    float uv[2];
    float color[4];
};

void insert_ft2_library(Command command) {
    FT2Library ft2_library;
    if (FT_Init_FreeType(&ft2_library.library)) {
        spdlog::error("Failed to initialize FreeType library");
    }
    command.insert_resource(ft2_library);
}
void create_renderer(
    Command command,
    Query<Get<Device, Queue, CommandPool>, With<RenderContext>> query
) {
    logger->set_level(spdlog::level::debug);
    if (!query.single().has_value()) return;
    auto [device, queue, command_pool] = query.single().value();
    TextRenderer text_renderer;
    text_renderer.text_descriptor_set_layout = DescriptorSetLayout::create(
        device,
        {vk::DescriptorSetLayoutBinding()
             .setBinding(1)
             .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
             .setDescriptorCount(1)
             .setStageFlags(vk::ShaderStageFlagBits::eFragment),
         vk::DescriptorSetLayoutBinding()
             .setBinding(0)
             .setDescriptorType(vk::DescriptorType::eUniformBuffer)
             .setDescriptorCount(1)
             .setStageFlags(vk::ShaderStageFlagBits::eVertex)}
    );
    auto pool_sizes = std::vector<vk::DescriptorPoolSize>{
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
    };
    text_renderer.text_descriptor_pool = DescriptorPool::create(
        device, vk::DescriptorPoolCreateInfo()
                    .setMaxSets(1)
                    .setPoolSizes(pool_sizes)
                    .setFlags(
                        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet |
                        vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT
                    )
    );
    text_renderer.text_descriptor_set = DescriptorSet::create(
        device, vk::DescriptorSetAllocateInfo()
                    .setDescriptorPool(*text_renderer.text_descriptor_pool)
                    .setSetLayouts({*text_renderer.text_descriptor_set_layout})
    );
    text_renderer.text_vertex_buffer = Buffer::create_host(
        device, 1024, vk::BufferUsageFlagBits::eTransferSrc
    );
    text_renderer.text_texture_staging_buffer = Buffer::create_host(
        device, 1024 * 1024 * 4, vk::BufferUsageFlagBits::eTransferSrc
    );
    text_renderer.text_texture_image = Image::create_2d(
        device, 1024, 1024, 1, 1, vk::Format::eR8G8B8A8Srgb,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );
    text_renderer.text_texture_image_view = ImageView::create_2d(
        device, text_renderer.text_texture_image, vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor
    );
    // transition image layout
    auto command_buffer = CommandBuffer::allocate_primary(device, command_pool);
    command_buffer->begin(vk::CommandBufferBeginInfo{});
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(vk::ImageLayout::eUndefined);
    barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(text_renderer.text_texture_image);
    barrier.setSubresourceRange(
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );
    barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
    command_buffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier
    );
    command_buffer->end();
    auto submit_info = vk::SubmitInfo().setCommandBuffers(*command_buffer);
    queue->submit(submit_info, nullptr);
    command.spawn(text_renderer);
}
void destroy_renderer(
    Query<Get<Device>, With<RenderContext>> query,
    Resource<FT2Library> ft2_library,
    Query<Get<TextRenderer>> text_renderer_query
) {
    if (!text_renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device] = query.single().value();
    auto [text_renderer] = text_renderer_query.single().value();
    logger->debug("destroy text renderer");
    text_renderer.text_descriptor_set.destroy(
        device, text_renderer.text_descriptor_pool
    );
    text_renderer.text_descriptor_pool.destroy(device);
    text_renderer.text_descriptor_set_layout.destroy(device);
    text_renderer.text_vertex_buffer.destroy(device);
    text_renderer.text_texture_staging_buffer.destroy(device);
    text_renderer.text_texture_image_view.destroy(device);
    text_renderer.text_texture_image.destroy(device);
    FT_Done_FreeType(ft2_library->library);
}
}  // namespace systems
}  // namespace font
}  // namespace pixel_engine