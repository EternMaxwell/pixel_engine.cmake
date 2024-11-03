#include "pixel_engine/render_vk.h"

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include <vk_mem_alloc.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

using namespace pixel_engine::prelude;
using namespace pixel_engine::render_vk;
using namespace pixel_engine::render_vk::components;
using namespace pixel_engine::render_vk::systems;

static std::shared_ptr<spdlog::logger> logger =
    spdlog::default_logger()->clone("render_vk");

void systems::create_context(
    Command cmd,
    Query<
        Get<window::components::Window>,
        With<window::components::PrimaryWindow>> query,
    Resource<RenderVKPlugin> plugin
) {
    if (!query.single().has_value()) {
        return;
    }
    auto [window] = query.single().value();
    Instance instance =
        Instance::create("Pixel Engine", VK_MAKE_VERSION(0, 1, 0), logger);
    PhysicalDevice physical_device = PhysicalDevice::create(instance);
    Device device                  = Device::create(instance, physical_device);
    Surface surface = Surface::create(instance, window.get_handle());
    Swapchain swap_chain =
        Swapchain::create(physical_device, device, surface, window.m_vsync);
    CommandPool command_pool = CommandPool::create(device);
    Queue queue              = Queue::create(device);
    cmd.spawn(
        instance, physical_device, device, surface, swap_chain, queue,
        command_pool, RenderContext{}
    );
    CommandBuffer cmd_buffer =
        CommandBuffer::allocate_primary(device, command_pool);
    Fence fence = Fence::create(
        device,
        vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled)
    );
    cmd.spawn(cmd_buffer, fence, ContextCommandBuffer{});
}

void systems::recreate_swap_chain(
    Query<Get<PhysicalDevice, Device, Surface, Swapchain>, With<RenderContext>>
        query
) {
    if (!query.single().has_value()) {
        return;
    }
    auto [physical_device, device, surface, swap_chain] =
        query.single().value();
    swap_chain.recreate(physical_device, device, surface);
}

void systems::get_next_image(
    Query<Get<Device, Swapchain, CommandPool, Queue>, With<RenderContext>>
        query,
    Query<Get<CommandBuffer, Fence>, With<ContextCommandBuffer>> cmd_query
) {
    if (!query.single().has_value()) return;
    if (!cmd_query.single().has_value()) return;
    auto [cmd_buffer, cmd_fence]                   = cmd_query.single().value();
    auto [device, swap_chain, command_pool, queue] = query.single().value();
    auto image = swap_chain.next_image(device);
    device->waitForFences(*cmd_fence, VK_TRUE, UINT64_MAX);
    device->resetFences(*cmd_fence);
    auto res = device->waitForFences(swap_chain.fence(), VK_TRUE, UINT64_MAX);
    device->resetFences(swap_chain.fence());
    cmd_buffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    cmd_buffer->begin(vk::CommandBufferBeginInfo{});
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(vk::ImageLayout::ePresentSrcKHR);
    barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image);
    barrier.setSubresourceRange(
        vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
    );
    cmd_buffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, {barrier}
    );
    cmd_buffer->clearColorImage(
        image, vk::ImageLayout::eTransferDstOptimal,
        vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
        vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
    );
    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
    barrier.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal);
    barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
    cmd_buffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, {barrier}
    );
    cmd_buffer->end();
    auto submit_info = vk::SubmitInfo().setCommandBuffers(*cmd_buffer);
    queue->submit(submit_info, cmd_fence);
}

void systems::present_frame(
    Query<Get<Swapchain, Queue, Device, CommandPool>, With<RenderContext>>
        query,
    Query<Get<CommandBuffer, Fence>, With<ContextCommandBuffer>> cmd_query
) {
    if (!query.single().has_value()) return;
    if (!cmd_query.single().has_value()) return;
    auto [cmd_buffer, cmd_fence]                   = cmd_query.single().value();
    auto [swap_chain, queue, device, command_pool] = query.single().value();
    device->waitForFences(*cmd_fence, VK_TRUE, UINT64_MAX);
    device->resetFences(*cmd_fence);
    cmd_buffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    cmd_buffer->begin(vk::CommandBufferBeginInfo{});
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
    barrier.setNewLayout(vk::ImageLayout::ePresentSrcKHR);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(swap_chain.current_image());
    barrier.setSubresourceRange(
        vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
    );
    cmd_buffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, {barrier}
    );
    cmd_buffer->end();
    auto submit_info = vk::SubmitInfo().setCommandBuffers(*cmd_buffer);
    queue->submit(submit_info, cmd_fence);
    try {
        auto ret =
            queue->presentKHR(vk::PresentInfoKHR()
                                  .setWaitSemaphores({})
                                  .setSwapchains(*swap_chain)
                                  .setImageIndices(swap_chain.image_index));
    } catch (vk::OutOfDateKHRError& e) {
        logger->warn("Swap chain out of date: {}", e.what());
    } catch (std::exception& e) {}
}

void systems::destroy_context(
    Command cmd,
    Query<
        Get<Instance, Device, Surface, Swapchain, CommandPool>,
        With<RenderContext>> query,
    Query<Get<CommandBuffer, Fence>, With<ContextCommandBuffer>> cmd_query
) {
    if (!query.single().has_value()) return;
    if (!cmd_query.single().has_value()) return;
    auto [cmd_buffer, cmd_fence] = cmd_query.single().value();
    auto [instance, device, surface, swap_chain, command_pool] =
        query.single().value();
    device->waitIdle();
    swap_chain.destroy(device);
    surface.destroy(instance);
    cmd_buffer.free(device, command_pool);
    cmd_fence.destroy(device);
    command_pool.destroy(device);
    device.destroy();
    instance.destroy();
}