#include "font_gl_test.h"

int main() {
    using namespace pixel_engine;
    using namespace font_gl_test;
    using namespace prelude;

    App app = App::create();
    app.enable_loop()
        .add_plugin(WindowPlugin{})
        .add_plugin(AssetServerGLPlugin{})
        .add_plugin(RenderGLPlugin{})
        .add_plugin(font_gl::FontGLPlugin{})
        .add_plugin(TestPlugin{})
        .run();
}