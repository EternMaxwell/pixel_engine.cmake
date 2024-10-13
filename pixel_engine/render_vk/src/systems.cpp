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
        Get<window::components::WindowHandle>,
        With<window::components::PrimaryWindow>> query,
    Resource<RenderVKPlugin> plugin
) {
    if (!query.single().has_value()) {
        return;
    }
    auto [window_handle] = query.single().value();
    spdlog::info("Creating Vulkan instance");
    Instance instance =
        Instance::create("Pixel Engine", VK_MAKE_VERSION(0, 1, 0), logger);
    PhysicalDevice physical_device = PhysicalDevice::create(instance);
    Device device = Device::create(instance, physical_device);
    Surface surface = Surface::create(instance, window_handle.window_handle);
    Swapchain swap_chain = Swapchain::create(
        physical_device, device, surface, window_handle.vsync
    );
    CommandPool command_pool = CommandPool::create(device);
    Queue queue = Queue::create(device);
    cmd.spawn(
        instance, physical_device, device, surface, swap_chain, queue,
        command_pool, RenderContext{}
    );
}

void systems::recreate_swap_chain(
    Command cmd,
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
    Command cmd, Query<Get<Device, Swapchain>, With<RenderContext>> query
) {
    if (!query.single().has_value()) {
        return;
    }
    auto [device, swap_chain] = query.single().value();
    auto image = swap_chain.next_image(device);
}

void systems::present_frame(
    Command cmd, Query<Get<Swapchain, Queue>, With<RenderContext>> query
) {
    if (!query.single().has_value()) {
        return;
    }
    auto [swap_chain, queue] = query.single().value();
    try {
        auto ret = queue->presentKHR(
            vk::PresentInfoKHR()
                .setWaitSemaphores(swap_chain.render_finished())
                .setSwapchains(*swap_chain)
                .setImageIndices(swap_chain.image_index)
        );
    } catch (vk::OutOfDateKHRError& e) {
        logger->warn("Swap chain out of date: {}", e.what());
    } catch (std::exception& e) {}
}

void systems::destroy_context(
    Command cmd,
    Query<
        Get<Instance, Device, Surface, Swapchain, CommandPool>,
        With<RenderContext>> query
) {
    spdlog::info("Destroying render context");
    if (!query.single().has_value()) {
        return;
    }
    auto [instance, device, surface, swap_chain, command_pool] =
        query.single().value();
    swap_chain.destroy(device);
    surface.destroy(instance);
    command_pool.destroy(device);
    device.destroy();
    instance.destroy();
}