#pragma once

#include "font/components.h"
#include "font/resources.h"
#include "font/systems.h"

namespace pixel_engine {
namespace font {
using namespace prelude;
struct FontPlugin : Plugin {
    uint32_t canvas_width  = 4096;
    uint32_t canvas_height = 1024;
    void build(App& app) override {
        auto render_vk_plugin = app.get_plugin<render_vk::RenderVKPlugin>();
        if (!render_vk_plugin) {
            throw std::runtime_error("FontPlugin requires RenderVKPlugin");
        }
        app.add_system(PreStartup, systems::vulkan::insert_ft2_library)
            .after(render_vk::systems::create_context);
        app.add_system(Startup, systems::vulkan::create_renderer);
        app.add_system(Render, systems::vulkan::draw_text);
        app.add_system(Exit, systems::vulkan::destroy_renderer);
    }
};
}  // namespace font
}  // namespace pixel_engine