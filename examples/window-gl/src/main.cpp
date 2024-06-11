#include <pixel_engine/prelude.h>

#include <filesystem>
#include <iostream>
#include <string>

#include "pipeline.h"

int main() {
    using namespace pixel_engine;
    using namespace prelude;
    using namespace pipeline_test;
    using namespace plugins;

    App app;
    app.add_plugin(LoopPlugin{})
        .add_plugin(WindowGLPlugin{})
        .add_plugin(AssetServerGLPlugin{})
        .add_plugin(RenderGLPlugin{})
        .add_plugin(PipelineTestPlugin{})
        .run_parallel();

    return 0;
}
