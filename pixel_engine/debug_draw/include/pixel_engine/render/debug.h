#pragma once

#include "draw_dbg/components.h"
#include "draw_dbg/systems.h"

namespace pixel_engine::render::debug {
namespace vulkan {
using namespace pixel_engine::prelude;
struct DebugRenderPlugin : Plugin {
    void build(App& app) override {
        app.add_system(Startup, systems::create_line_drawer);
        app.add_system(Startup, systems::create_point_drawer);
        app.add_system(Startup, systems::create_triangle_drawer);
        app.add_system(Exit, systems::destroy_line_drawer);
        app.add_system(Exit, systems::destroy_point_drawer);
        app.add_system(Exit, systems::destroy_triangle_drawer);
    }
};
}  // namespace vulkan
}  // namespace pixel_engine::render::debug