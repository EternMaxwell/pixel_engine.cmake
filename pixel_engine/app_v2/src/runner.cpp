#include "pixel_engine/app/runner.h"

using namespace pixel_engine::app;

Runner::Runner(
    spp::sparse_hash_map<const type_info*, std::unique_ptr<SubApp>>* sub_apps
)
    : m_sub_apps(sub_apps),
      m_pools(std::make_unique<WorkerPool>()),
      m_sets(std::make_unique<SetMap>()),
      m_control_pool(std::make_unique<BS::thread_pool>(4)) {
    add_worker(
        "default", std::clamp(std::thread::hardware_concurrency(), 4u, 16u)
    );
    add_worker("single", 1);
}
Runner::StageNode::StageNode(
    const type_info* stage, std::unique_ptr<StageRunner>&& runner
)
    : stage(stage),
      runner(std::forward<std::unique_ptr<StageRunner>>(runner)) {}
void Runner::StageNode::clear_tmp() {
    weak_prev_stages.clear();
    weak_next_stages.clear();
}
size_t Runner::StageNode::get_depth() {
    if (depth) return depth.value();
    depth = 0;
    for (auto& prev : strong_prev_stages) {
        if (auto ptr = prev.lock()) {
            depth = std::max(depth.value(), ptr->get_depth() + 1);
        }
    }
    return depth.value();
}
bool Runner::stage_startup(const type_info* stage) {
    return m_startup_stages.find(stage) != m_startup_stages.end();
}
bool Runner::stage_loop(const type_info* stage) {
    return m_loop_stages.find(stage) != m_loop_stages.end();
}
bool Runner::stage_state_transition(const type_info* stage) {
    return m_state_transition_stages.find(stage) !=
           m_state_transition_stages.end();
}
bool Runner::stage_exit(const type_info* stage) {
    return m_exit_stages.find(stage) != m_exit_stages.end();
}
void Runner::build() {
    for (auto& [ptr, stage] : m_startup_stages) {
        stage->runner->build();
        for (auto next_ptr : stage->next_stages) {
            auto it = m_startup_stages.find(next_ptr);
            if (it != m_startup_stages.end()) {
                stage->strong_next_stages.insert(it->second);
                it->second->strong_prev_stages.insert(stage);
            }
        }
        for (auto prev_ptr : stage->prev_stages) {
            auto it = m_startup_stages.find(prev_ptr);
            if (it != m_startup_stages.end()) {
                stage->strong_prev_stages.insert(it->second);
                it->second->strong_next_stages.insert(stage);
            }
        }
    }
    std::vector<std::shared_ptr<StageNode>> startup_stages;
    for (auto& [ptr, stage] : m_startup_stages) {
        startup_stages.push_back(stage);
    }
    std::sort(
        startup_stages.begin(), startup_stages.end(),
        [](const auto& a, const auto& b) {
            return a->get_depth() < b->get_depth();
        }
    );
    for (size_t i = 0; i < startup_stages.size(); ++i) {
        for (size_t j = i + 1; j < startup_stages.size(); ++j) {
            if (startup_stages[i]->runner->m_dst !=
                    startup_stages[j]->runner->m_dst &&
                startup_stages[i]->runner->m_dst !=
                    startup_stages[j]->runner->m_src &&
                startup_stages[j]->runner->m_dst !=
                    startup_stages[i]->runner->m_src &&
                startup_stages[i]->runner->m_dst !=
                    startup_stages[j]->runner->m_src) {
                startup_stages[i]->weak_next_stages.insert(startup_stages[j]);
                startup_stages[j]->weak_prev_stages.insert(startup_stages[i]);
            }
        }
    }
    for (auto& [ptr, stage] : m_loop_stages) {
        stage->runner->build();
        for (auto next_ptr : stage->next_stages) {
            auto it = m_loop_stages.find(next_ptr);
            if (it != m_loop_stages.end()) {
                stage->strong_next_stages.insert(it->second);
                it->second->strong_prev_stages.insert(stage);
            }
        }
        for (auto prev_ptr : stage->prev_stages) {
            auto it = m_loop_stages.find(prev_ptr);
            if (it != m_loop_stages.end()) {
                stage->strong_prev_stages.insert(it->second);
                it->second->strong_next_stages.insert(stage);
            }
        }
    }
    std::vector<std::shared_ptr<StageNode>> loop_stages;
    for (auto& [ptr, stage] : m_loop_stages) {
        loop_stages.push_back(stage);
    }
    std::sort(
        loop_stages.begin(), loop_stages.end(),
        [](const auto& a, const auto& b) {
            return a->get_depth() < b->get_depth();
        }
    );
    for (size_t i = 0; i < loop_stages.size(); ++i) {
        for (size_t j = i + 1; j < loop_stages.size(); ++j) {
            if (loop_stages[i]->runner->m_dst !=
                    loop_stages[j]->runner->m_dst &&
                loop_stages[i]->runner->m_dst !=
                    loop_stages[j]->runner->m_src &&
                loop_stages[j]->runner->m_dst !=
                    loop_stages[i]->runner->m_src &&
                loop_stages[i]->runner->m_dst !=
                    loop_stages[j]->runner->m_src) {
                loop_stages[i]->weak_next_stages.insert(loop_stages[j]);
                loop_stages[j]->weak_prev_stages.insert(loop_stages[i]);
            }
        }
    }
    for (auto& [ptr, stage] : m_state_transition_stages) {
        stage->runner->build();
        for (auto next_ptr : stage->next_stages) {
            auto it = m_state_transition_stages.find(next_ptr);
            if (it != m_state_transition_stages.end()) {
                stage->strong_next_stages.insert(it->second);
                it->second->strong_prev_stages.insert(stage);
            }
        }
        for (auto prev_ptr : stage->prev_stages) {
            auto it = m_state_transition_stages.find(prev_ptr);
            if (it != m_state_transition_stages.end()) {
                stage->strong_prev_stages.insert(it->second);
                it->second->strong_next_stages.insert(stage);
            }
        }
    }
    std::vector<std::shared_ptr<StageNode>> state_transition_stages;
    for (auto& [ptr, stage] : m_state_transition_stages) {
        state_transition_stages.push_back(stage);
    }
    std::sort(
        state_transition_stages.begin(), state_transition_stages.end(),
        [](const auto& a, const auto& b) {
            return a->get_depth() < b->get_depth();
        }
    );
    for (size_t i = 0; i < state_transition_stages.size(); ++i) {
        for (size_t j = i + 1; j < state_transition_stages.size(); ++j) {
            if (state_transition_stages[i]->runner->m_dst !=
                    state_transition_stages[j]->runner->m_dst &&
                state_transition_stages[i]->runner->m_dst !=
                    state_transition_stages[j]->runner->m_src &&
                state_transition_stages[j]->runner->m_dst !=
                    state_transition_stages[i]->runner->m_src &&
                state_transition_stages[i]->runner->m_dst !=
                    state_transition_stages[j]->runner->m_src) {
                state_transition_stages[i]->weak_next_stages.insert(
                    state_transition_stages[j]
                );
                state_transition_stages[j]->weak_prev_stages.insert(
                    state_transition_stages[i]
                );
            }
        }
    }
    for (auto& [ptr, stage] : m_exit_stages) {
        stage->runner->build();
        for (auto next_ptr : stage->next_stages) {
            auto it = m_exit_stages.find(next_ptr);
            if (it != m_exit_stages.end()) {
                stage->strong_next_stages.insert(it->second);
                it->second->strong_prev_stages.insert(stage);
            }
        }
        for (auto prev_ptr : stage->prev_stages) {
            auto it = m_exit_stages.find(prev_ptr);
            if (it != m_exit_stages.end()) {
                stage->strong_prev_stages.insert(it->second);
                it->second->strong_next_stages.insert(stage);
            }
        }
    }
    std::vector<std::shared_ptr<StageNode>> exit_stages;
    for (auto& [ptr, stage] : m_exit_stages) {
        exit_stages.push_back(stage);
    }
    std::sort(
        exit_stages.begin(), exit_stages.end(),
        [](const auto& a, const auto& b) {
            return a->get_depth() < b->get_depth();
        }
    );
    for (size_t i = 0; i < exit_stages.size(); ++i) {
        for (size_t j = i + 1; j < exit_stages.size(); ++j) {
            if (exit_stages[i]->runner->m_dst !=
                    exit_stages[j]->runner->m_dst &&
                exit_stages[i]->runner->m_dst !=
                    exit_stages[j]->runner->m_src &&
                exit_stages[j]->runner->m_dst !=
                    exit_stages[i]->runner->m_src &&
                exit_stages[i]->runner->m_dst !=
                    exit_stages[j]->runner->m_src) {
                exit_stages[i]->weak_next_stages.insert(exit_stages[j]);
                exit_stages[j]->weak_prev_stages.insert(exit_stages[i]);
            }
        }
    }
}

void Runner::bake_all() {
    for (auto& [ptr, stage] : m_startup_stages) {
        stage->runner->bake();
    }
    for (auto& [ptr, stage] : m_loop_stages) {
        stage->runner->bake();
    }
    for (auto& [ptr, stage] : m_state_transition_stages) {
        stage->runner->bake();
    }
    for (auto& [ptr, stage] : m_exit_stages) {
        stage->runner->bake();
    }
}

void Runner::run(std::shared_ptr<StageNode> node) {
    if (node->runner->m_src == nullptr) {
        msg_queue.push(node);
        return;
    }
    auto ftr = m_control_pool->submit_task([this, node]() {
        node->runner->run();
        msg_queue.push(node);
    });
}

void Runner::run_startup() {
    for (auto& [ptr, stage] : m_startup_stages) {
        stage->prev_count =
            stage->strong_prev_stages.size() + stage->weak_prev_stages.size();
    }
    size_t m_remain  = m_startup_stages.size();
    size_t m_running = 0;
    for (auto& [ptr, stage] : m_startup_stages) {
        if (stage->prev_count == 0) {
            run(stage);
            ++m_running;
        }
    }
    while (m_running > 0) {
        msg_queue.wait();
        auto ptr = msg_queue.pop();
        --m_remain;
        --m_running;
        for (auto& next_weak : ptr->strong_next_stages) {
            if (auto next = next_weak.lock()) {
                --next->prev_count;
                if (next->prev_count == 0) {
                    run(next);
                    ++m_running;
                }
            }
        }
        for (auto& next_weak : ptr->weak_next_stages) {
            if (auto next = next_weak.lock()) {
                --next->prev_count;
                if (next->prev_count == 0) {
                    run(next);
                    ++m_running;
                }
            }
        }
    }
    if (m_remain > 0) {
        spdlog::warn("Startup stages have circular dependencies");
    }
}

void Runner::run_loop() {
    for (auto& [ptr, stage] : m_loop_stages) {
        stage->prev_count =
            stage->strong_prev_stages.size() + stage->weak_prev_stages.size();
    }
    size_t m_remain  = m_loop_stages.size();
    size_t m_running = 0;
    for (auto& [ptr, stage] : m_loop_stages) {
        if (stage->prev_count == 0) {
            run(stage);
            ++m_running;
        }
    }
    while (m_running > 0) {
        msg_queue.wait();
        auto ptr = msg_queue.pop();
        --m_remain;
        --m_running;
        for (auto& next_weak : ptr->strong_next_stages) {
            if (auto next = next_weak.lock()) {
                --next->prev_count;
                if (next->prev_count == 0) {
                    run(next);
                    ++m_running;
                }
            }
        }
        for (auto& next_weak : ptr->weak_next_stages) {
            if (auto next = next_weak.lock()) {
                --next->prev_count;
                if (next->prev_count == 0) {
                    run(next);
                    ++m_running;
                }
            }
        }
    }
    if (m_remain > 0) {
        spdlog::warn("Loop stages have circular dependencies");
    }
}

void Runner::run_state_transition() {
    for (auto& [ptr, stage] : m_state_transition_stages) {
        stage->prev_count =
            stage->strong_prev_stages.size() + stage->weak_prev_stages.size();
    }
    size_t m_remain  = m_state_transition_stages.size();
    size_t m_running = 0;
    for (auto& [ptr, stage] : m_state_transition_stages) {
        if (stage->prev_count == 0) {
            run(stage);
            ++m_running;
        }
    }
    while (m_running > 0) {
        msg_queue.wait();
        auto ptr = msg_queue.pop();
        --m_remain;
        --m_running;
        for (auto& next_weak : ptr->strong_next_stages) {
            if (auto next = next_weak.lock()) {
                --next->prev_count;
                if (next->prev_count == 0) {
                    run(next);
                    ++m_running;
                }
            }
        }
        for (auto& next_weak : ptr->weak_next_stages) {
            if (auto next = next_weak.lock()) {
                --next->prev_count;
                if (next->prev_count == 0) {
                    run(next);
                    ++m_running;
                }
            }
        }
    }
    if (m_remain > 0) {
        spdlog::warn("State transition stages have circular dependencies");
    }
}

void Runner::run_exit() {
    for (auto& [ptr, stage] : m_exit_stages) {
        stage->prev_count =
            stage->strong_prev_stages.size() + stage->weak_prev_stages.size();
    }
    size_t m_remain  = m_exit_stages.size();
    size_t m_running = 0;
    for (auto& [ptr, stage] : m_exit_stages) {
        if (stage->prev_count == 0) {
            run(stage);
            ++m_running;
        }
    }
    while (m_running > 0) {
        msg_queue.wait();
        auto ptr = msg_queue.pop();
        --m_remain;
        --m_running;
        for (auto& next_weak : ptr->strong_next_stages) {
            if (auto next = next_weak.lock()) {
                --next->prev_count;
                if (next->prev_count == 0) {
                    run(next);
                    ++m_running;
                }
            }
        }
        for (auto& next_weak : ptr->weak_next_stages) {
            if (auto next = next_weak.lock()) {
                --next->prev_count;
                if (next->prev_count == 0) {
                    run(next);
                    ++m_running;
                }
            }
        }
    }
    if (m_remain > 0) {
        spdlog::warn("Exit stages have circular dependencies");
    }
}

void Runner::tick_events() {
    for (auto&& [type, subapp] : *m_sub_apps) {
        subapp->tick_events();
    }
}

void Runner::end_commands() {
    for (auto&& [type, subapp] : *m_sub_apps) {
        subapp->end_commands();
    }
}

void Runner::update_states() {
    for (auto&& [type, subapp] : *m_sub_apps) {
        subapp->update_states();
    }
}

void Runner::add_worker(const std::string& name, uint32_t num_threads) {
    m_pools->add_pool(name, num_threads);
}