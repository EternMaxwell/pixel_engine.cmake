#pragma once

#include <pixel_engine/app.h>
#include <pixel_engine/render_vk.h>

#include "components.h"

namespace pixel_engine::render::pixel {
namespace systems {
using namespace pixel_engine::prelude;
using namespace pixel_engine::render_vk::components;
using namespace components;
void create_pixel_block_pipeline(
    Command command, Query<Get<Device, CommandPool>, With<RenderContext>> query
);
void create_pixel_pipeline(
    Command command, Query<Get<Device, CommandPool>, With<RenderContext>> query
);
void draw_pixel_blocks_vk(
    Query<Get<Device, CommandPool, Swapchain, Queue>, With<RenderContext>>
        query,
    Query<Get<PixelBlockRenderer>> renderer_query,
    Query<Get<const PixelBlock, const BlockPos2d>> pixel_block_query
);
void destroy_pixel_block_pipeline(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<PixelBlockRenderer>> renderer_query
);
void destroy_pixel_pipeline(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<PixelRenderer>> renderer_query
);
}  // namespace systems
}  // namespace pixel_engine::render::pixel