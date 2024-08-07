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

void create_pipeline(Command command, Resource<AssetServerGL> asset_server);

void create_render_pass(
    Command command,
    Query<
        Get<entt::entity, Sprite>, With<Transform>,
        Without<pipeline::RenderPass>>
        sprite_query,
    Query<Get<entt::entity>, With<SpritePipeline>> pipeline_query);

void draw(
    Query<Get<const Sprite, const Transform>> sprite_query,
    Query<
        Get<const Transform, const OrthoProjection>, With<Camera2d>, Without<>>
        camera_query,
    Query<
        Get<const Pipeline, Buffers, Images>,
        With<SpritePipeline, ProgramLinked>, Without<>>
        pipeline_query);
}  // namespace systems
}  // namespace sprite_render_gl
}  // namespace pixel_engine