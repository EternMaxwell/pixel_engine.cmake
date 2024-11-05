#include <pixel_engine/prelude.h>

#include "task_queue_test.h"

int main() {
    using namespace pixel_engine;
    using namespace prelude;

    App app = App::create();
    app.enable_loop()
        .add_plugin(WindowPlugin{})
        .add_plugin(RenderGLPlugin{})
        .add_plugin(task_queue::TaskQueuePlugin{})
        .add_plugin(test_queue_test::TestPlugin{})
        .run();
}