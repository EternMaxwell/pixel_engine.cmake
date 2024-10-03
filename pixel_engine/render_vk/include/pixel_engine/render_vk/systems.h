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
void recreate_swap_chain(Command cmd, Resource<RenderContext> context);
void get_next_image(Resource<RenderContext> context);
void present_frame(Resource<RenderContext> context);
void destroy_context(Command cmd, Resource<RenderContext> context);
}  // namespace systems
}  // namespace render_vk
}  // namespace pixel_engine