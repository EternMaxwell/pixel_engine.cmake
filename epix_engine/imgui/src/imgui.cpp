#include "pixel_engine/imgui.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::imgui;

EPIX_API void ImGuiPluginVK::build(App& app) {
    app.add_system(PreStartup, systems::insert_imgui_ctx);
    app.add_system(Startup, systems::init_imgui).use_worker("single");
    app.add_system(PreRender, systems::begin_imgui)
        .after(pixel_engine::render_vk::systems::get_next_image);
    app.add_system(PostRender, systems::end_imgui)
        .before(pixel_engine::render_vk::systems::present_frame);
    app.add_system(Exit, systems::deinit_imgui).use_worker("single");
}
