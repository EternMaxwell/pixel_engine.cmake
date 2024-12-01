#pragma once

#include "pixel_engine/entity.h"
#include "pixel_engine/font.h"
#include "pixel_engine/input.h"
#include "pixel_engine/render/debug.h"
#include "pixel_engine/render/pixel.h"
#include "pixel_engine/render_vk.h"
#include "pixel_engine/sprite.h"
#include "pixel_engine/window.h"

namespace pixel_engine {
namespace prelude {
// plugins
using WindowPlugin        = window::WindowPlugin;
using RenderVKPlugin      = render_vk::RenderVKPlugin;
}  // namespace prelude
}  // namespace pixel_engine