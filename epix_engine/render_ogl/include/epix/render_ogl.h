#pragma once

#include "render_ogl/systems.h"

namespace epix::render::ogl {
using namespace prelude;
struct RenderGlPlugin : Plugin {
    EPIX_API void build(App& app) override;
};
}  // namespace epix::render::ogl