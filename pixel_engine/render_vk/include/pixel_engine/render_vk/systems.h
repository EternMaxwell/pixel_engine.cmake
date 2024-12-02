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

struct ContextCommandBuffer {};

EPIX_API void create_context(
    Command cmd,
    Query<
        Get<window::components::Window>,
        With<window::components::PrimaryWindow>> query,
    Res<RenderVKPlugin> plugin
);
EPIX_API void recreate_swap_chain(
    Query<Get<PhysicalDevice, Device, Surface, Swapchain>, With<RenderContext>>
        query
);
EPIX_API void get_next_image(
    Query<Get<Device, Swapchain, CommandPool, Queue>, With<RenderContext>>
        query,
    Query<Get<CommandBuffer, Fence>, With<ContextCommandBuffer>> cmd_query
);
EPIX_API void present_frame(
    Query<Get<Swapchain, Queue, Device, CommandPool>, With<RenderContext>>
        query,
    Query<Get<CommandBuffer, Fence>, With<ContextCommandBuffer>> cmd_query
);
EPIX_API void destroy_context(
    Command cmd,
    Query<
        Get<Instance, Device, Surface, Swapchain, CommandPool>,
        With<RenderContext>> query,
    Query<Get<CommandBuffer, Fence>, With<ContextCommandBuffer>> cmd_query
);
}  // namespace systems
}  // namespace render_vk
}  // namespace pixel_engine