#include "pixel_engine/render_vk.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::render_vk;

void RenderVKPlugin::build(App& app) {
    auto window_plugin = app.get_plugin<window::WindowPlugin>();
    window_plugin->primary_desc().set_vsync(vsync).set_hints(
        {{GLFW_RESIZABLE, GLFW_TRUE}, {GLFW_CLIENT_API, GLFW_NO_API}}
    );

    app.add_system(PreStartup, systems::create_context)
        .in_set(window::WindowStartUpSets::after_window_creation)
        .use_worker("single");
    app.add_system(Prepare, systems::recreate_swap_chain).use_worker("single");
    app.add_system(PreRender, systems::get_next_image)
        .use_worker("single")
        .after(systems::recreate_swap_chain);
    app.add_system(PostRender, systems::present_frame).use_worker("single");
    app.add_system(PostExit, systems::destroy_context).use_worker("single");
}