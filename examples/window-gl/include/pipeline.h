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
    using namespace render_gl::components;

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
        command.spawn(core_components::Camera2dBundle{});
    }

    void create_sprite(Command command, Resource<AssetServerGL> asset_server) {
        using namespace sprite_render_gl::components;

        command.spawn(SpriteBundle{
            .sprite{
                .texture = asset_server->load_image_2d("../assets/textures/test.png"),
                .sampler = asset_server->create_sampler(GL_CLAMP, GL_CLAMP, GL_LINEAR, GL_LINEAR),
            },
            .transform{
                .translation = {0.0f, 0.0f, 0.0f},
            },
        });
    }

    void move_sprite(Query<Get<sprite_render_gl::components::Transform>, Without<>> sprite_query) {
        for (auto [transform] : sprite_query.iter()) {
            transform.translation.x += 0.000001f;
            transform.translation.y += 0.000001f;
        }
    }

    void camera_ortho_to_primary_window(
        Query<Get<core_components::Camera2d, core_components::OrthoProjection>, Without<>> camera_query,
        Query<Get<const WindowSize, const PrimaryWindow>, Without<>> window_query) {
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
                .add_system_main(
                    Startup{}, create_sprite,
                    after(
                        app.get_plugin<render_gl::RenderGLPlugin>()->context_creation_node,
                        app.get_plugin<AssetServerGLPlugin>()->insert_asset_server_node))
                .add_system(Update{}, camera_ortho_to_primary_window)
                .add_system(Update{}, move_sprite);
        }
    };
}  // namespace pipeline_test