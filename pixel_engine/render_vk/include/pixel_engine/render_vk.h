#pragma once

#include "render_vk/components.h"
#include "render_vk/resources.h"
#include "render_vk/systems.h"
#include "render_vk/vulkan_headers.h"

namespace pixel_engine {
namespace render_vk {
using namespace prelude;
struct RenderVKPlugin : Plugin {
    bool vsync = false;
    void build(App& app) override {
        auto window_plugin = app.get_plugin<window::WindowPlugin>();
        window_plugin->primary_desc().set_vsync(vsync).set_hints(
            {{GLFW_RESIZABLE, GLFW_TRUE}, {GLFW_CLIENT_API, GLFW_NO_API}}
        );

        app.add_system(PreStartup, systems::create_context)
            .in_set(window::WindowStartUpSets::after_window_creation)
            .use_worker("single");
        app.add_system(Prepare, systems::recreate_swap_chain)
            .use_worker("single");
        app.add_system(PreRender, systems::get_next_image)
            .use_worker("single")
            .after(systems::recreate_swap_chain);
        app.add_system(PostRender, systems::present_frame)
            .use_worker("single");
        app.add_system(PostExit, systems::destroy_context)
            .use_worker("single");
    }
};
}  // namespace render_vk
}  // namespace pixel_engine
