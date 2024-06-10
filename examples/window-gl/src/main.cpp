#include <pixel_engine/entity.h>
#include <pixel_engine/plugins/window_gl.h>
#include <pixel_engine/prelude.h>

int main() {
    using namespace pixel_engine;
    using namespace prelude;

    spdlog::set_level(spdlog::level::debug);

    App app;
    app.add_plugin(LoopPlugin{}).add_plugin(WindowGLPlugin{}).run_parallel();

    return 0;
}
