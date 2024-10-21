#pragma once

#include <pixel_engine/app.h>
#include <pixel_engine/window.h>
#include <spdlog/spdlog.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include "components.h"
#include "resources.h"

namespace pixel_engine {
namespace render_vk {
struct RenderVKPlugin;
namespace systems {
using namespace pixel_engine::prelude;
using namespace components;
using namespace window::components;

void create_context(
    Command cmd,
    Query<
        Get<window::components::WindowHandle>,
        With<window::components::PrimaryWindow>> query,
    Resource<RenderVKPlugin> plugin
);
void recreate_swap_chain(
    Command cmd,
    Query<Get<PhysicalDevice, Device, Surface, Swapchain>, With<RenderContext>>
        query
);
void get_next_image(
    Command cmd,
    Query<Get<Device, Swapchain, CommandPool, Queue>, With<RenderContext>> query
);
void present_frame(
    Command cmd, Query<Get<Swapchain, Queue>, With<RenderContext>> query
);
void destroy_context(
    Command cmd,
    Query<
        Get<Instance, Device, Surface, Swapchain, CommandPool>,
        With<RenderContext>> query
);
}  // namespace systems
}  // namespace render_vk
}  // namespace pixel_engine