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
        app.add_system(PreStartup, systems::insert_ft2_library);
        app.add_system(Startup, systems::create_renderer);
        app.add_system(Render, systems::draw_text);
        app.add_system(Exit, systems::destroy_renderer);
    }
};
}  // namespace font
}  // namespace pixel_engine