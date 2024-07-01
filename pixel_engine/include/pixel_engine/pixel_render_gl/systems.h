#pragma once

#include <glad/glad.h>

#include "components.h"
#include "pixel_engine/asset_server_gl/asset_server_gl.h"
#include "pixel_engine/camera/components.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/render_gl/render_gl.h"

namespace pixel_engine {
    namespace pixel_render_gl {
        namespace systems {
            using namespace prelude;
            using namespace asset_server_gl::resources;
            using namespace render_gl::components;
            using namespace components;
            using namespace camera;

            void create_pipeline(Command command, Resource<AssetServerGL> asset_server);

            void draw(
                Query<Get<const Pixels, const PixelSize, const Transform>> pixels_query,
                Query<Get<const Transform, const OrthoProjection>, With<Camera2d>, Without<>> camera_query,
                Query<Get<Pipeline, Buffers, Images>, With<PixelPipeline, ProgramLinked>, Without<>> pipeline_query);
        }  // namespace systems
    }      // namespace pixel_render_gl
}  // namespace pixel_engine