#pragma once

#include "render_vk/components.h"
#include "render_vk/resources.h"
#include "render_vk/systems.h"
#include "render_vk/vulkan_headers.h"

namespace pixel_engine {
namespace render_vk {
using namespace prelude;
struct RenderVKPlugin : Plugin {
    bool vsync = true;
    void build(App& app) override {
        app.add_system(systems::create_context)
            .in_stage(app::PreStartup)
            .in_set(window::WindowStartUpSets::after_window_creation)
            .use_worker("single");
        app.add_system(systems::recreate_swap_chain)
            .in_stage(app::Prepare)
            .use_worker("single");
        app.add_system(systems::get_next_image)
            .in_stage(app::PreRender)
            .use_worker("single")
            .after(systems::recreate_swap_chain);
        app.add_system(systems::present_frame)
            .in_stage(app::PostRender)
            .use_worker("single");
        app.add_system(systems::destroy_context)
            .in_stage(app::PostShutdown)
            .use_worker("single");
    }
};
}  // namespace render_vk
}  // namespace pixel_engine
