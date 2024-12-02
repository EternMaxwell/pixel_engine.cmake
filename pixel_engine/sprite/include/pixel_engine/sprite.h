#pragma once

#include <pixel_engine/font.h>

#include "sprite/components.h"
#include "sprite/resources.h"
#include "sprite/systems.h"

namespace pixel_engine {
namespace sprite {
using namespace prelude;
using namespace render_vk::components;
using namespace sprite::components;
using namespace sprite::resources;
using namespace sprite::systems;

struct SpritePluginVK : Plugin {
    EPIX_API void build(App& app) override;
};
}  // namespace sprite
}  // namespace pixel_engine