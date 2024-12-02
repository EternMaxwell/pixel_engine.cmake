#pragma once

#include "render_ogl/systems.h"

namespace pixel_engine::render::ogl {
using namespace prelude;
struct RenderGlPlugin : Plugin {
    EPIX_API void build(App& app) override;
};
}  // namespace pixel_engine::render::ogl