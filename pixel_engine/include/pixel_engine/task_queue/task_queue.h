﻿#pragma once

#include "resources.h"

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
            TaskQueuePlugin() {}
            TaskQueuePlugin(int num_threads) : num_threads(num_threads) {}

            void build(App& app) {
                app.configure_sets(TaskQueueSets::insert_task_queue, TaskQueueSets::after_insertion)
                    .add_system(PreStartup{}, insert_task_queue, in_set(TaskQueueSets::insert_task_queue));
            }
        };
    }  // namespace task_queue
}  // namespace pixel_engine
