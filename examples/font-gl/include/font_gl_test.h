#pragma once

#include <pixel_engine/font_gl/font.h>
#include <pixel_engine/prelude.h>

namespace font_gl_test {
    using namespace pixel_engine;
    using namespace pixel_engine::font_gl;
    using namespace pixel_engine::prelude;
    using namespace window::components;

    void create_camera(Command command) { command.spawn(Camera2dBundle{}); }

    void create_text(Command command, Resource<AssetServerGL> asset_server, Resource<FT2Library> library) {
        command.spawn(TextBundle{
            .text = Text{
                .text = U"hello, font.",
                .size = 0,
                .pixels = 64,
                .antialias = false,
                .center{0.5f, 0.5f},
                .font_face = asset_server->load_font(
                    library->library, "../assets/fonts/HachicroUndertaleBattleFontRegular-L3zlg.ttf"),
            }});
    }

    void camera_ortho_to_primary_window(
        Query<Get<Camera2d, OrthoProjection>, Without<>> query,
        Query<Get<WindowSize, PrimaryWindow>, Without<>> projection_query) {
        for (auto [proj] : query.iter()) {
            for (auto [size] : projection_query.iter()) {
                float ratio = (float)size.width / size.height;
                proj.left = -ratio;
                proj.right = ratio;
            }
        }
    }

    class TestPlugin : public Plugin {
       public:
        void build(App& app) {
            app.add_system(Startup{}, create_camera)
                .add_system(
                    Startup{}, create_text,
                    after(
                        app.get_plugin<FontGLPlugin>()->library_insert_node,
                        app.get_plugin<AssetServerGLPlugin>()->insert_asset_server_node))
                .add_system(PreRender{}, camera_ortho_to_primary_window);
        }
    };
}  // namespace font_gl_test