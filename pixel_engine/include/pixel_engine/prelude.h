#pragma once

#include "pixel_engine/asset_server_gl/asset_server_gl.h"
#include "pixel_engine/camera/components.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/font.h"
#include "pixel_engine/font_gl/font.h"
#include "pixel_engine/input.h"
#include "pixel_engine/pixel_render_gl/pixel_render_gl.h"
#include "pixel_engine/render/debug.h"
#include "pixel_engine/render/pixel.h"
#include "pixel_engine/render_gl/render_gl.h"
#include "pixel_engine/render_vk.h"
#include "pixel_engine/sprite.h"
#include "pixel_engine/sprite_render_gl/sprite_render.h"
#include "pixel_engine/task_queue/task_queue.h"
#include "pixel_engine/transform/components.h"
#include "pixel_engine/window.h"

namespace pixel_engine {
namespace prelude {
// plugins
using AssetServerGLPlugin = asset_server_gl::AssetServerGLPlugin;
using WindowPlugin        = window::WindowPlugin;
using RenderGLPlugin      = render_gl::RenderGLPlugin;
using RenderVKPlugin      = render_vk::RenderVKPlugin;
}  // namespace prelude
}  // namespace pixel_engine