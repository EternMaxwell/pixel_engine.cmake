#pragma once

#include <epix/render_vk.h>

#include "components.h"

namespace epix::render::debug::vulkan {
struct DebugRenderPlugin;
}  // namespace epix::render::debug::vulkan

namespace epix::render::debug::vulkan::systems {
using namespace epix::prelude;
using namespace epix::render_vk::components;
using namespace epix::render::debug::vulkan::components;
EPIX_API void create_line_drawer(
    Query<Get<Device, CommandPool>, With<RenderContext>> query,
    Command cmd,
    Res<DebugRenderPlugin> plugin
);
EPIX_API void create_point_drawer(
    Query<Get<Device, CommandPool>, With<RenderContext>> query,
    Command cmd,
    Res<DebugRenderPlugin> plugin
);
EPIX_API void create_triangle_drawer(
    Query<Get<Device, CommandPool>, With<RenderContext>> query,
    Command cmd,
    Res<DebugRenderPlugin> plugin
);
EPIX_API void destroy_line_drawer(
    Query<Get<Device>, With<RenderContext>> device_query,
    Query<Get<LineDrawer>> query
);
EPIX_API void destroy_point_drawer(
    Query<Get<Device>, With<RenderContext>> device_query,
    Query<Get<PointDrawer>> query
);
EPIX_API void destroy_triangle_drawer(
    Query<Get<Device>, With<RenderContext>> device_query,
    Query<Get<TriangleDrawer>> query
);
}  // namespace epix::render::debug::vulkan::systems