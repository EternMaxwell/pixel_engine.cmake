#pragma once

#include <pixel_engine/render_vk.h>

#include "components.h"

namespace pixel_engine::render::debug::vulkan::systems {
using namespace pixel_engine::prelude;
using namespace pixel_engine::render_vk::components;
using namespace pixel_engine::render::debug::vulkan::components;
void create_line_drawer(
    Query<Get<Device, CommandPool>, With<RenderContext>> query, Command cmd
);
void create_point_drawer(
    Query<Get<Device, CommandPool>, With<RenderContext>> query, Command cmd
);
void create_triangle_drawer(
    Query<Get<Device, CommandPool>, With<RenderContext>> query, Command cmd
);
void destroy_line_drawer(
    Query<Get<Device>, With<RenderContext>> device_query,
    Query<Get<LineDrawer>> query
);
void destroy_point_drawer(
    Query<Get<Device>, With<RenderContext>> device_query,
    Query<Get<PointDrawer>> query
);
void destroy_triangle_drawer(
    Query<Get<Device>, With<RenderContext>> device_query,
    Query<Get<TriangleDrawer>> query
);
}  // namespace pixel_engine::render::debug::vulkan::systems