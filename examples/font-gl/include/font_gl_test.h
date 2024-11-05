#pragma once

#include <pixel_engine/font_gl/font.h>
#include <pixel_engine/prelude.h>

namespace font_gl_test {
using namespace pixel_engine;
using namespace pixel_engine::font_gl;
using namespace pixel_engine::prelude;
using namespace window::components;
using namespace camera::components;
using namespace transform::components;
using namespace prelude;
using namespace asset_server_gl::resources;
using namespace font_gl::resources;
using namespace font_gl::components;

void create_camera(Command command) { command.spawn(Camera2dBundle{}); }

void create_text(
    Command command,
    ResMut<AssetServerGL> asset_server,
    ResMut<FT2Library> library
) {
    command.spawn(TextBundle{
        .text =
            Text{
                .text = U"hello, font.",
                .size = 0,
                .pixels = 64,
                .antialias = false,
                .center{0.5f, 0.5f},
                .font_face = asset_server->load_font(
                    library->library,
                    "../assets/fonts/"
                    "HachicroUndertaleBattleFontRegular-L3zlg.ttf"
                ),
            }
    });
}

void camera_ortho_to_primary_window(
    Query<Get<OrthoProjection>, With<Camera2d>> query,
    Query<Get<const Window>, With<PrimaryWindow>> projection_query
) {
    for (auto [proj] : query.iter()) {
        for (auto [window] : projection_query.iter()) {
            auto size = window.get_size();
            float ratio = (float)size.width / size.height;
            proj.left = -ratio;
            proj.right = ratio;
        }
    }
}

class TestPlugin : public Plugin {
   public:
    void build(App& app) {
        app.add_system(Startup, create_camera)
            ->add_system(Startup, create_text)
            ->add_system(PreRender, camera_ortho_to_primary_window);
    }
};
}  // namespace font_gl_test