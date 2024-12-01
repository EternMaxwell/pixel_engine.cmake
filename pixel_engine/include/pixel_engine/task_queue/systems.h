#pragma once

#include "resources.h"
#include "task_queue.h"

namespace pixel_engine {
namespace task_queue {
class TaskQueuePlugin;

namespace systems {
using namespace prelude;

EPIX_API void insert_task_queue(
    Command command, ResMut<TaskQueuePlugin> task_queue
);
}  // namespace systems
}  // namespace task_queue
}  // namespace pixel_engine