#pragma once

#include <pixel_engine/prelude.h>

namespace test_queue_test {
    using namespace pixel_engine;
    using namespace prelude;
    using namespace task_queue;

    class TestPlugin : public Plugin {
       private:
        std::function<void(Resource<TaskQueue>)> test_fun = [&](Resource<TaskQueue> task_queue) {
            auto& pool = task_queue->get_pool();
            pool->detach_task([]() { std::cout << "Hello, World!" << std::endl; });
            pool->wait();
        };

       public:
        TestPlugin() {}

        void build(App& app) { app.add_system(Update{}, test_fun); }
    };
}  // namespace test_queue_test