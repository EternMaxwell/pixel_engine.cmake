#include <pixel_engine/prelude.h>

#include <filesystem>
#include <iostream>
#include <string>

#include "pipeline.h"

int main() {
    using namespace pixel_engine;
    using namespace prelude;
    using namespace pipeline_test;

    App app;
    app.enable_loop()
        .add_plugin(WindowPlugin{})
        .add_plugin(AssetServerGLPlugin{})
        .add_plugin(RenderGLPlugin{})
        .add_plugin(sprite_render_gl::SpriteRenderGLPlugin{})
        .add_plugin(pixel_render_gl::PixelRenderGLPlugin{})
        .add_plugin(TestPlugin{})
        .run();

    return 0;
}
