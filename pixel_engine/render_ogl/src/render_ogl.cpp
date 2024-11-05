#include "pixel_engine/render_ogl.h"

namespace pixel_engine::render::ogl {
void RenderGlPlugin::build(App& app) {
    app.add_system(PreStartup, systems::context_creation)
        .use_worker("single")
        .after(window::systems::init_glfw);
    app.add_system(Prepare, systems::context_creation).use_worker("single");
    app.add_system(Prepare, systems::clear_color)
        .use_worker("single")
        .after(systems::context_creation);
    app.add_system(Prepare, systems::update_viewport)
        .before(systems::clear_color)
        .use_worker("single")
        .after(systems::context_creation);
    app.add_system(PostRender, systems::swap_buffers).use_worker("single");
}
}  // namespace pixel_engine::render::ogl