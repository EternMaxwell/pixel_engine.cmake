#pragma once
#include "resources.h"
#include "systems.h"

namespace pixel_engine {
namespace task_queue {
using namespace prelude;
using namespace resources;

enum class TaskQueueSets {
    insert_task_queue,
    after_insertion,
};

class TaskQueuePlugin : public Plugin {
   private:
    int num_threads = 4;

    std::function<void(Command)> insert_task_queue = [&](Command command) {
        command.insert_resource(TaskQueue(num_threads));
    };

   public:
    EPIX_API TaskQueuePlugin() {}
    EPIX_API TaskQueuePlugin(int num_threads) : num_threads(num_threads) {}

    EPIX_API void build(App& app);
};
}  // namespace task_queue
}  // namespace pixel_engine
