#include <epix/app.h>
#include <epix/render_ogl.h>
#include <epix/window.h>

using namespace epix;
using namespace epix::prelude;

int main() {
    App app = App::create();
    app.add_plugin(window::WindowPlugin());
    app.add_plugin(render::ogl::RenderGlPlugin());
    app.run();
}