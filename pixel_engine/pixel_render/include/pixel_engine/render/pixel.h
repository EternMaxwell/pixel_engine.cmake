#pragma once

#include "pixel/components.h"
#include "pixel/systems.h"

namespace pixel_engine::render::pixel {
using namespace pixel_engine::prelude;
struct PixelRenderPlugin : Plugin {
    EPIX_API void build(App& app) override;
};
}  // namespace pixel_engine::render::pixel