#include "pixel_engine/entity.h"

bool pixel_engine::entity::check_exit(EventReader<AppExit> exit_events) {
    //spdlog::debug("Check if exit");
    for (auto& e : exit_events.read()) {
        spdlog::info("Exit event received, exiting app.");
        return true;
    }
    return false;
}

void pixel_engine::entity::exit_app(EventWriter<AppExit> exit_events) { exit_events.write(AppExit{}); }

size_t pixel_engine::entity::SystemNode::user_before_depth() {
    size_t max_depth = 0;
    for (auto& before : user_defined_before) {
        max_depth = std::max(max_depth, before->user_before_depth() + 1);
    }
    return max_depth;
}

double pixel_engine::entity::SystemNode::time_to_reach() {
    double max_time = 0;
    for (auto& before : user_defined_before) {
        max_time = std::max(max_time, before->time_to_reach() + before->system->get_avg_time());
    }
    return max_time;
}

void pixel_engine::entity::SystemRunner::run() {
    runned = true;
    std::atomic<int> tasks_in = 0;
    while (!all_load()) {
        auto next = get_next();
        if (next != nullptr) {
            if (!next->in_main_thread) {
                {
                    std::unique_lock<std::mutex> lk(m);
                    cv.wait(lk, [&] { return tasks_in <= pool->get_thread_count(); });
                }
                tasks_in++;
                if (ignore_scheduler) {
                    futures[next] = pool->submit_task([&, next] {
                        bool condition_pass = true;
                        for (auto& condition : next->conditions) {
                            if (!condition->if_run(app)) {
                                condition_pass = false;
                                break;
                            }
                        }
                        if (condition_pass) {
                            next->system->run();
                        }
                        tasks_in--;
                        std::unique_lock<std::mutex> lk(m);
                        any_done = true;
                        cv.notify_all();
                        return;
                    });
                } else {
                    App* app = this->app;
                    futures[next] = pool->submit_task([&, app, next] {
                        if (next->scheduler->should_run(app)) {
                            bool condition_pass = true;
                            for (auto& condition : next->conditions) {
                                if (!condition->if_run(app)) {
                                    condition_pass = false;
                                    break;
                                }
                            }
                            if (condition_pass) {
                                next->system->run();
                            }
                        }
                        tasks_in--;
                        std::unique_lock<std::mutex> lk(m);
                        any_done = true;
                        cv.notify_all();
                        return;
                    });
                }
            } else {
                std::promise<void> p;
                futures[next] = p.get_future();
                next->system->run();
                p.set_value();
            }
        }
    }
}

void pixel_engine::entity::SystemRunner::prepare() {
    tails.clear();
    if (!runned) {
        sort_depth();
    } else {
        sort_time();
    }
    for (auto& sys : systems_all) {
        sys->app_generated_before.clear();
        for (auto& ins : tails) {
            if (ins->system->contrary_to(sys->system) &&
                (ins->user_defined_before.find(sys) == ins->user_defined_before.end())) {
                sys->app_generated_before.insert(ins);
            }
        }
        tails.insert(sys);
    }
    for (auto& sys : systems_all) {
        for (auto& before : sys->app_generated_before) {
            tails.erase(before);
        }
        for (auto& before : sys->user_defined_before) {
            tails.erase(before);
        }
    }
}

std::shared_ptr<pixel_engine::entity::SystemNode> pixel_engine::entity::SystemRunner::should_run(
    const std::shared_ptr<SystemNode>& sys) {
    bool before_done = true;

    // user def before
    for (auto& before : sys->user_defined_before) {
        // if before not in futures
        if (futures.find(before) == futures.end()) {
            return should_run(before);
        }
        // else if in and finished
        else if (futures[before].wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            before_done = false;
        }
    }
    // app gen before
    for (auto& before : sys->app_generated_before) {
        // if before not in futures
        if (futures.find(before) == futures.end()) {
            return should_run(before);
        }
        // else if in and finished
        else if (futures[before].wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            before_done = false;
        }
    }
    if (before_done) return sys;
    return nullptr;
}

std::shared_ptr<pixel_engine::entity::SystemNode> pixel_engine::entity::SystemRunner::get_next() {
    for (auto& sys : tails) {
        if (futures.find(sys) != futures.end()) {
            continue;
        }
        auto should = should_run(sys);
        if (should != nullptr) {
            return should;
        }
    }
    std::unique_lock<std::mutex> lk(m);
    if (!any_done) {
        cv.wait_for(lk, std::chrono::milliseconds(2), [&] {
            if (any_done) {
                any_done = false;
                return true;
            }
            return false;
        });
    } else {
        any_done = false;
    }
    return nullptr;
}

bool pixel_engine::entity::SystemRunner::done() {
    for (auto& sys : tails) {
        if (futures.find(sys) == futures.end()) {
            return false;
        }
        if (futures[sys].wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            return false;
        }
    }
    return true;
}

bool pixel_engine::entity::SystemRunner::all_load() {
    for (auto& sys : tails) {
        if (futures.find(sys) == futures.end()) {
            return false;
        }
    }
    return true;
}

void pixel_engine::entity::App::end_commands() {
    for (auto& cmd : m_existing_commands) {
        cmd.end();
    }
    m_existing_commands.clear();
}

void pixel_engine::entity::App::tick_events() {
    for (auto& [key, value] : m_events) {
        while (!value->empty()) {
            auto& event = value->front();
            if (event.ticks >= 1) {
                value->pop_front();
            } else {
                break;
            }
        }
        for (auto& event : *value) {
            event.ticks++;
        }
    }
}

void pixel_engine::entity::App::update_states() {
    for (auto& state : m_state_update) {
        state->run();
    }
}

void pixel_engine::entity::App::check_locked(std::shared_ptr<SystemNode> node, std::shared_ptr<SystemNode>& node2) {
    for (auto& before : node->user_defined_before) {
        if (before == node2) {
            throw std::runtime_error("System locked.");
        }
        check_locked(before, node2);
    }
}

void pixel_engine::entity::App::run() {
    spdlog::info("App started.");
    spdlog::info("Loading system runners.");
    load_runners<
        PreStartup, Startup, PostStartup, OnStateChange, PreUpdate, Update, PostUpdate, PreRender, Render, PostRender,
        PreExit, Exit, PostExit>();
    spdlog::info("Loading done.");
    spdlog::info("Preparing start up runners.");
    prepare_runners<PreStartup, Startup, PostStartup>();
    spdlog::info("Running start up runners.");
    run_runners<PreStartup, Startup, PostStartup>();
    // loop
    do {
        spdlog::debug("Prepare runners.");
        prepare_runners<OnStateChange, PreUpdate, Update, PostUpdate, PreRender, Render, PostRender>();
        spdlog::debug("Run on state change runners.");
        run_runner<OnStateChange>();
        spdlog::debug("Update states.");
        update_states();
        spdlog::debug("Run update runners.");
        run_runners<PreUpdate, Update, PostUpdate>();
        spdlog::debug("Run render runners.");
        run_runners<PreRender, Render, PostRender>();
        spdlog::debug("Tick events.");
        tick_events();
    } while (m_loop_enabled && !run_system_v(std::function(check_exit)));
    // prepare exit runners.
    prepare_runners<PreExit, Exit, PostExit>();
    run_runners<PreExit, Exit, PostExit>();
    spdlog::info("App terminated.");
}
