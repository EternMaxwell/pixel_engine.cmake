#pragma once

#include "epix/entity.h"
#include "epix/font.h"
#include "epix/input.h"
#include "epix/render/debug.h"
#include "epix/render/pixel.h"
#include "epix/render_vk.h"
#include "epix/sprite.h"
#include "epix/window.h"
#include "epix/world/sand.h"

namespace epix {
namespace prelude {
// plugins
using WindowPlugin   = window::WindowPlugin;
using RenderVKPlugin = render_vk::RenderVKPlugin;
}  // namespace prelude
}  // namespace epix