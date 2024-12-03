#pragma once

#include <pixel_engine/render_vk.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

#include "components.h"
#include "resources.h"

namespace pixel_engine {
namespace font {
struct FontPlugin;
namespace systems {
namespace vulkan {
using namespace prelude;
using namespace font::components;
using namespace render_vk::components;
using namespace font::resources;
EPIX_API void insert_ft2_library(
    Command command, Query<Get<Device>, With<RenderContext>> query
);
EPIX_API void create_renderer(
    Command command,
    Query<Get<Device, Queue, CommandPool>, With<RenderContext>> query,
    Res<FontPlugin> font_plugin,
    ResMut<resources::vulkan::FT2Library> ft2_library
);
EPIX_API void draw_text(
    Query<Get<TextRenderer>> text_renderer_query,
    Query<Get<Text, TextPos>> text_query,
    ResMut<resources::vulkan::FT2Library> ft2_library,
    Query<Get<Device, Queue, CommandPool, Swapchain>, With<RenderContext>>
        swapchain_query,
    Res<FontPlugin> font_plugin
);
EPIX_API void destroy_renderer(
    Query<Get<Device>, With<RenderContext>> query,
    ResMut<resources::vulkan::FT2Library> ft2_library,
    Query<Get<TextRenderer>> text_renderer_query
);
}  // namespace vulkan
namespace ogl {
using namespace prelude;
using namespace font::components;
using namespace font::resources;
EPIX_API void create_text_renderer(Command command);
EPIX_API void destroy_text_renderer(
    Query<Get<TextRendererGl>> text_renderer_query
);
}  // namespace ogl
}  // namespace systems
}  // namespace font
}  // namespace pixel_engine