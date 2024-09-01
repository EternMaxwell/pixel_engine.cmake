#pragma once

#include "resources.h"
#include "task_queue.h"

namespace pixel_engine {
namespace task_queue {
class TaskQueuePlugin;

namespace systems {
using namespace prelude;

void insert_task_queue(Command command, Resource<TaskQueuePlugin> task_queue) {
    command.insert_resource(resources::TaskQueue());
}
}  // namespace systems
}  // namespace task_queue
}  // namespace pixel_engine