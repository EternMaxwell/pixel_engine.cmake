#include "pixel_engine/task_queue/resources.h"
#include "pixel_engine/task_queue/task_queue.h"

EPIX_API void pixel_engine::task_queue::systems::insert_task_queue(
    Command command, ResMut<TaskQueuePlugin> task_queue
) {
    command.insert_resource(resources::TaskQueue());
}

EPIX_API std::shared_ptr<BS::thread_pool>
pixel_engine::task_queue::resources::TaskQueue::get_pool() {
    for (auto& p : pool) {
        // return if the pool is not busy
        if (p->wait_for(std::chrono::seconds(0))) {
            return p;
        }
    }
    pool.push_back(std::make_shared<BS::thread_pool>(num_threads));
    return pool.back();
}

EPIX_API std::shared_ptr<BS::thread_pool>
pixel_engine::task_queue::resources::TaskQueue::get_pool(int index) {
    if (index < pool.size()) {
        // create pools if the index is out of bounds
        for (int i = pool.size(); i <= index; i++) {
            pool.push_back(std::make_shared<BS::thread_pool>(num_threads));
        }
    }
    return pool[index];
}

EPIX_API std::shared_ptr<BS::thread_pool>
pixel_engine::task_queue::resources::TaskQueue::request_pool() {
    for (auto& p : pool) {
        // return if the pool is not busy
        if (p->wait_for(std::chrono::seconds(0))) {
            return p;
        }
    }
    return nullptr;
}

EPIX_API std::shared_ptr<BS::thread_pool>
pixel_engine::task_queue::resources::TaskQueue::request_pool(int index) {
    if (index < pool.size()) {
        return nullptr;
    }
    return pool[index];
}

EPIX_API void pixel_engine::task_queue::TaskQueuePlugin::build(App& app) {
    app.configure_sets(
           TaskQueueSets::insert_task_queue, TaskQueueSets::after_insertion
    )
        .add_system(PreStartup, systems::insert_task_queue)
        .in_set(TaskQueueSets::insert_task_queue);
}
