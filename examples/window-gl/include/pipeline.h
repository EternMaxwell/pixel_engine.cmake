#pragma once

#include <pixel_engine/prelude.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pipeline_test {
using namespace pixel_engine;
using namespace prelude;
using namespace asset_server_gl::resources;
using namespace window::components;
using namespace render_gl::systems;

struct TestPipeline {};

#pragma pack(push, 1)
struct Vertex {
    float position[3];
    float color[3];
    float tex_coords[2];
};
#pragma pack(pop)

void insert_camera(Command command) {
    spdlog::debug("Inserting camera");
    command.spawn(camera::Camera2dBundle{});
}

void create_pixels(Command command) {
    command.spawn(pixel_render_gl::components::PixelGroupBundle{.pixels{
        .pixels = {
            {.position = {0.0f, 0.0f, 0.0f}, .color = {1.0f, 0.0f, 0.0f, 1.0f}},
            {.position = {1.0f, 0.0f, 0.0f}, .color = {0.0f, 1.0f, 0.0f, 1.0f}},
            {.position = {0.0f, 1.0f, 0.0f}, .color = {0.0f, 0.0f, 1.0f, 1.0f}},
        }}});
}

void create_sprite(Command command, Resource<AssetServerGL> asset_server) {
    using namespace sprite_render_gl::components;

    command.spawn(SpriteBundle{
        .sprite{
            .texture{
                .texture =
                    asset_server->load_image_2d("../assets/textures/test.png"),
                .sampler = asset_server->create_sampler(
                    GL_CLAMP, GL_CLAMP, GL_LINEAR, GL_LINEAR),
            },
        },
        .transform{
            .translation = {0.0f, 0.0f, 0.0f},
        },
    });
}

void move_sprite(
    Query<Get<transform::Transform>, With<sprite_render_gl::components::Sprite>>
        sprite_query) {
    for (auto [transform] : sprite_query.iter()) {
        transform.translation.x += 0.000001f;
        transform.translation.y += 0.000001f;
    }
}

void camera_ortho_to_primary_window(
    Query<Get<transform::OrthoProjection>, With<camera::Camera2d>, Without<>>
        camera_query,
    Query<Get<const WindowSize>, With<PrimaryWindow>, Without<>> window_query) {
    for (auto [projection] : camera_query.iter()) {
        for (auto [window_size] : window_query.iter()) {
            float ratio = (float)window_size.width / (float)window_size.height;
            projection.left = -ratio;
            projection.right = ratio;
            return;
        }
    }
}

class TestPlugin : public Plugin {
   public:
    void build(App& app) override {
        SystemNode draw_node;

        app.add_system(Startup{}, insert_camera)
            .add_system_main(Startup{}, create_sprite)
            .add_system(Startup{}, create_pixels)
            .add_system(Update{}, camera_ortho_to_primary_window)
            .add_system(Update{}, move_sprite);
    }
};
}  // namespace pipeline_test