#include "pixel_engine/render_vk.h"

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1002000
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

void systems::create_context(
    Command cmd,
    Query<
        Get<window::components::WindowHandle>,
        With<window::components::PrimaryWindow>> query
) {
    if (!query.single().has_value()) {
        return;
    }
    spdlog::info("Creating render context");
    auto [window_handle] = query.single().value();
    RenderContext context;
    spdlog::info("Creating Vulkan instance");
    context.instance = Instance::create(
        "Pixel Engine", VK_MAKE_VERSION(1, 0, 0),
        spdlog::default_logger()->clone("render_vk")
    );
    spdlog::info("Creating device");
    context.device = Device::create(context.instance);
    spdlog::info("Creating swap chain");
    context.swap_chain =
        SwapChain::create(context.device, window_handle.window_handle);
    cmd.insert_resource(context);
}

void systems::recreate_swap_chain(
    Command cmd, Resource<RenderContext> context
) {
    context->swap_chain.recreate();
}

void systems::get_next_image(Resource<RenderContext> context) {
    try {
        context->swap_chain.next_image();
    } catch (const std::exception& e) {
        context->swap_chain.recreate();
        context->swap_chain.next_image();
    }
}

void systems::present_frame(Resource<RenderContext> context) {
    try {
        context->device.queue.presentKHR(
            vk::PresentInfoKHR()
                .setSwapchains(context->swap_chain.swapchain)
                .setPImageIndices(&context->swap_chain.image_index)
                .setWaitSemaphores(context->swap_chain.render_finished_semaphore
                )
        );
    } catch (const std::exception& e) {
        context->swap_chain.recreate();
    }
}

void systems::destroy_context(Command cmd, Resource<RenderContext> context) {
    spdlog::info("Destroying render context");
    context->swap_chain.destroy();
    context->device.destroy();
    context->instance.destroy();
}