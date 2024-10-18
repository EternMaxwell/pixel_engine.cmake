#pragma once

#include "font/components.h"
#include "font/resources.h"
#include "font/systems.h"

namespace pixel_engine {
namespace font {
using namespace prelude;
struct FontPlugin : Plugin {
    void build(App& app) override {
        auto render_vk_plugin = app.get_plugin<render_vk::RenderVKPlugin>();
        if (!render_vk_plugin) {
            throw std::runtime_error("FontPlugin requires RenderVKPlugin");
        }
        app.add_system(systems::insert_ft2_library).in_stage(app::PreStartup);
        app.add_system(systems::create_renderer).in_stage(app::Startup);
        app.add_system(systems::destroy_renderer).in_stage(app::Shutdown);
    }
};
}  // namespace font
}  // namespace pixel_engine