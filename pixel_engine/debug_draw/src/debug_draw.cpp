#include "pixel_engine/render/debug.h"

using namespace pixel_engine::render;

EPIX_API void debug::vulkan::DebugRenderPlugin::build(App& app) {
    app.add_system(Startup, systems::create_line_drawer);
    app.add_system(Startup, systems::create_point_drawer);
    app.add_system(Startup, systems::create_triangle_drawer);
    app.add_system(Exit, systems::destroy_line_drawer);
    app.add_system(Exit, systems::destroy_point_drawer);
    app.add_system(Exit, systems::destroy_triangle_drawer);
}