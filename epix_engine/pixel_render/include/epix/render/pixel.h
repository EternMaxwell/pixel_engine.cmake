#pragma once

#include "pixel/components.h"
#include "pixel/systems.h"

namespace epix::render::pixel {
using namespace epix::prelude;
struct PixelRenderPlugin : Plugin {
    EPIX_API void build(App& app) override;
};
}  // namespace epix::render::pixel