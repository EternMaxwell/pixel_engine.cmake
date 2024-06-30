#pragma once

#include "pixel_engine/asset_server_gl/asset_server_gl.h"
#include "pixel_engine/components/components.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/font_gl/font.h"
#include "pixel_engine/pixel_render_gl/pixel_render_gl.h"
#include "pixel_engine/render_gl/render_gl.h"
#include "pixel_engine/sprite_render_gl/sprite_render.h"
#include "pixel_engine/task_queue/task_queue.h"
#include "pixel_engine/window/window.h"

namespace pixel_engine {
    namespace prelude {
        // plugins
        using AssetServerGLPlugin = asset_server_gl::AssetServerGLPlugin;
        using WindowPlugin = window::WindowPlugin;
        using RenderGLPlugin = render_gl::RenderGLPlugin;
    }  // namespace prelude
}  // namespace pixel_engine