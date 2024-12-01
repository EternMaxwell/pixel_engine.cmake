#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include "components.h"
#include "pixel_engine/asset_server_gl/resources.h"
#include "pixel_engine/camera/components.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/render_gl/render_gl.h"

namespace pixel_engine {
namespace sprite_render_gl {
namespace systems {
using namespace prelude;
using namespace asset_server_gl::resources;
using namespace render_gl::components;
using namespace sprite_render_gl::components;
using namespace camera;

using sprite_query_type = Query<Get<Transform, Sprite>, With<>, Without<>>;

EPIX_API void create_pipeline(
    Command command, ResMut<AssetServerGL> asset_server
);

EPIX_API void draw_sprite(
    render_gl::PipelineQuery::query_type<SpritePipeline> pipeline_query,
    Query<
        Get<const Transform, const OrthoProjection>,
        With<Camera2d>,
        Without<>> camera_query,
    sprite_query_type sprite_query
);
}  // namespace systems
}  // namespace sprite_render_gl
}  // namespace pixel_engine