#pragma once

#include <pixel_engine/font.h>

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
        app.add_system(PreStartup, insert_sprite_server_vk);
        app.add_system(Startup, create_sprite_renderer_vk);
        app.add_system(PostStartup, create_sprite_depth_vk);
        app.add_system(Prepare, loading_actual_image);
        app.add_system(Prepare, creating_actual_sampler);
        app.add_system(Prepare, update_image_bindings)
            .after(loading_actual_image);
        app.add_system(Prepare, update_sampler_bindings)
            .after(creating_actual_sampler);
        app.add_system(PreRender, update_sprite_depth_vk);
        app.add_system(Render, draw_sprite_2d_vk)
            .before(pixel_engine::font::systems::vulkan::draw_text);
        app.add_system(
            Exit, destroy_sprite_server_vk_images
        );
        app.add_system(
            Exit, destroy_sprite_server_vk_samplers
        );
        app.add_system(Exit, destroy_sprite_depth_vk);
        app.add_system(Exit, destroy_sprite_renderer_vk);
    }
};
}  // namespace sprite
}  // namespace pixel_engine