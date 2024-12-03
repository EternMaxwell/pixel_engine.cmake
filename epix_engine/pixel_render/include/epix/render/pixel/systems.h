#pragma once

#include <epix/app.h>
#include <epix/render_vk.h>

#include "components.h"

namespace epix::render::pixel {
namespace systems {
using namespace epix::prelude;
using namespace epix::render_vk::components;
using namespace components;
EPIX_API void create_pixel_block_pipeline(
    Command command, Query<Get<Device, CommandPool>, With<RenderContext>> query
);
EPIX_API void create_pixel_pipeline(
    Command command, Query<Get<Device, CommandPool>, With<RenderContext>> query
);
EPIX_API void draw_pixel_blocks_vk(
    Query<Get<Device, CommandPool, Swapchain, Queue>, With<RenderContext>>
        query,
    Query<Get<PixelBlockRenderer>> renderer_query,
    Query<Get<const PixelBlock, const BlockPos2d>> pixel_block_query
);
EPIX_API void destroy_pixel_block_pipeline(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<PixelBlockRenderer>> renderer_query
);
EPIX_API void destroy_pixel_pipeline(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<PixelRenderer>> renderer_query
);
}  // namespace systems
}  // namespace epix::render::pixel