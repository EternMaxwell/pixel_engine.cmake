#pragma once

#include "draw_dbg/components.h"
#include "draw_dbg/systems.h"

namespace pixel_engine::render::debug {
namespace vulkan {
using namespace pixel_engine::prelude;
struct DebugRenderPlugin : Plugin {
    size_t max_vertex_count = 2048 * 256;
    size_t max_model_count  = 2048;
    EPIX_API void build(App& app) override;
};
}  // namespace vulkan
}  // namespace pixel_engine::render::debug