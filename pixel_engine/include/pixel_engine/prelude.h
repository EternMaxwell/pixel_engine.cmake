#pragma once

#include "pixel_engine/components/components.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/plugins/asset_server_gl.h"
#include "pixel_engine/plugins/sprite_render_gl.h"
#include "pixel_engine/render_gl/render_gl.h"
#include "pixel_engine/window/window.h"

namespace pixel_engine {
    namespace prelude {
        namespace assetGL = plugins::asset_server_gl;

        // plugins
        using WindowPlugin = window::WindowPlugin;
        using RenderGLPlugin = render_gl::RenderGLPlugin;
    }  // namespace prelude
}  // namespace pixel_engine