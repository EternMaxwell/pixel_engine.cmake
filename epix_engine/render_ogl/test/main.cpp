#include <pixel_engine/app.h>
#include <pixel_engine/render_ogl.h>
#include <pixel_engine/window.h>

using namespace pixel_engine;
using namespace pixel_engine::prelude;

int main() {
    App app = App::create();
    app.add_plugin(window::WindowPlugin());
    app.add_plugin(render::ogl::RenderGlPlugin());
    app.run();
}