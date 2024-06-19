#include <pixel_engine/prelude.h>

#include "task_queue_test.h"

int main() {
    using namespace pixel_engine;
    using namespace prelude;

    App app;
    app.add_plugin(LoopPlugin{})
        .add_plugin(WindowPlugin{})
        .add_plugin(RenderGLPlugin{})
        .add_plugin(task_queue::TaskQueuePlugin{})
        .add_plugin(test_queue_test::TestPlugin{})
        .run_parallel();
}