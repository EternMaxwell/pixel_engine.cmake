#include "path.h"

using namespace path;

int main() {
    App app;
    app.add_plugin(LoopPlugin())
        .add_plugin(WindowPlugin())
        .add_plugin(AssetServerGLPlugin())
        .add_plugin(RenderGLPlugin())
        .add_plugin(PathPlugin())
        .run();
}