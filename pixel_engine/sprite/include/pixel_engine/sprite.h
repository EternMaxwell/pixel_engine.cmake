#pragma once

#include "sprite/components.h"
#include "sprite/resources.h"
#include "sprite/systems.h"

namespace pixel_engine {
namespace sprite {
using namespace prelude;
using namespace render_vk::components;
using namespace sprite::components;
using namespace sprite::resources;
using namespace sprite::systems;

struct SpritePluginVK : Plugin {
    void build(App& app) override {
        app.add_system(app::StartStage::PreStartup, insert_sprite_server_vk);
        app.add_system(app::StartStage::Startup, create_sprite_renderer_vk);
        app.add_system(app::RenderStage::Prepare, loading_actual_image);
        app.add_system(app::RenderStage::Prepare, creating_actual_sampler);
        app.add_system(
            app::ExitStage::Shutdown, destroy_sprite_server_vk_images
        );
        app.add_system(app::ExitStage::Shutdown, destroy_sprite_renderer_vk);
    }
};
}  // namespace sprite
}  // namespace pixel_engine