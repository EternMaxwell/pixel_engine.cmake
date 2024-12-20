﻿#pragma once

#include <freetype/freetype.h>
#include <ft2build.h>

#include <glm/gtc/type_ptr.hpp>

#include "components.h"
#include "pixel_engine/asset_server_gl/asset_server_gl.h"
#include "pixel_engine/camera/components.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/render_gl/render_gl.h"
#include "pixel_engine/transform/components.h"

namespace pixel_engine {
namespace font_gl {
namespace systems {
using namespace prelude;
using namespace resources;
using namespace render_gl::components;
using namespace asset_server_gl::resources;
using namespace components;
using namespace transform;
using namespace camera;

void insert_ft2_library(Command command);
void create_pipeline(Command command, ResMut<AssetServerGL> asset_server);
void draw(
    Query<Get<const Text, const Transform>> text_query,
    Query<
        Get<const Transform, const OrthoProjection>, With<Camera2d>, Without<>>
        camera_query,
    Query<
        Get<const window::Window>,
        With<window::PrimaryWindow>>
        window_query,
    render_gl::PipelineQuery::query_type<TextPipeline> pipeline_query);
}  // namespace systems
}  // namespace font_gl
}  // namespace pixel_engine