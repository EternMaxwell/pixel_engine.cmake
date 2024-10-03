#include <spdlog/spdlog.h>

#include "pixel_engine/app.h"
#include "pixel_engine/app/app.h"
#include "pixel_engine/app/command.h"
#include "pixel_engine/app/event.h"
#include "pixel_engine/app/query.h"
#include "pixel_engine/app/resource.h"
#include "pixel_engine/app/system.h"
#include "pixel_engine/app/tools.h"
#include "pixel_engine/app/world.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine;
using namespace pixel_engine::app;

app::Schedule prelude::PreStartup() {
    return app::Schedule(app::StartStage::PreStartup);
}
app::Schedule prelude::Startup() {
    return app::Schedule(app::StartStage::Startup);
}
app::Schedule prelude::PostStartup() {
    return app::Schedule(app::StartStage::PostStartup);
}
app::Schedule prelude::First() { return app::Schedule(app::LoopStage::First); }
app::Schedule prelude::PreUpdate() {
    return app::Schedule(app::LoopStage::PreUpdate);
}
app::Schedule prelude::Update() {
    return app::Schedule(app::LoopStage::Update);
}
app::Schedule prelude::PostUpdate() {
    return app::Schedule(app::LoopStage::PostUpdate);
}
app::Schedule prelude::Last() { return app::Schedule(app::LoopStage::Last); }
app::Schedule prelude::Prepare() {
    return app::Schedule(app::RenderStage::Prepare);
}
app::Schedule prelude::PreRender() {
    return app::Schedule(app::RenderStage::PreRender);
}
app::Schedule prelude::Render() {
    return app::Schedule(app::RenderStage::Render);
}
app::Schedule prelude::PostRender() {
    return app::Schedule(app::RenderStage::PostRender);
}
app::Schedule prelude::PreShutdown() {
    return app::Schedule(app::ExitStage::PreShutdown);
}
app::Schedule prelude::Shutdown() {
    return app::Schedule(app::ExitStage::Shutdown);
}
app::Schedule prelude::PostShutdown() {
    return app::Schedule(app::ExitStage::PostShutdown);
}

bool SystemNode::condition_pass(SubApp* src, SubApp* dst) {
    bool pass = true;
    if (m_schedule.m_condition) {
        pass &= m_schedule.m_condition->run(src, dst);
    }
    if (!pass) return false;
    for (auto& cond : m_conditions) {
        pass &= cond->run(src, dst);
        if (!pass) return false;
    }
    return true;
}
void SystemNode::run(SubApp* src, SubApp* dst) {
    if (m_system) m_system->run(src, dst);
}
double SystemNode::reach_time() {
    if (m_reach_time.has_value()) {
        return m_reach_time.value();
    } else {
        double max_time = 0.0;
        for (auto system : m_systems_before) {
            if (auto system_ptr = system.lock()) {
                max_time = std::max(
                    max_time, system_ptr->reach_time() +
                                  system_ptr->m_system->get_avg_time()
                );
            }
        }
        m_reach_time = max_time;
        return max_time;
    }
}
void SystemNode::reset_reach_time() { m_reach_time.reset(); }
void SystemNode::clear_temp() {
    m_temp_before.clear();
    m_temp_after.clear();
}

void SubStageRunner::build(
    const std::unordered_map<void*, std::shared_ptr<SystemNode>>& systems
) {
    build_logger->debug(
        "Building stage {} : {:d}.", m_stage_type_hash->name(), m_stage_value
    );
    m_systems.clear();
    for (auto& [id, sets] : *m_sets) {
        std::vector<std::vector<std::weak_ptr<SystemNode>>> set_systems;
        for (auto set : sets) {
            std::vector<std::weak_ptr<SystemNode>> ss;
            set_systems.push_back(ss);
            auto& set_system = set_systems.back();
            for (auto& [addr, system] : systems) {
                if (system->m_schedule.m_stage_type_hash == m_stage_type_hash &&
                    system->m_schedule.m_stage_value == m_stage_value &&
                    std::find_if(
                        system->m_in_sets.begin(), system->m_in_sets.end(),
                        [=](const auto& sset) {
                            return sset.m_set_type_hash == id &&
                                   sset.m_set_value == set;
                        }
                    ) != system->m_in_sets.end()) {
                    set_system.push_back(system);
                }
            }
        }
        for (size_t i = 0; i < set_systems.size(); i++) {
            for (size_t j = i + 1; j < set_systems.size(); j++) {
                for (auto& system_i : set_systems[i]) {
                    for (auto& system_j : set_systems[j]) {
                        system_i.lock()->m_system_ptrs_after.insert(
                            system_j.lock()->m_func_addr
                        );
                        system_j.lock()->m_system_ptrs_before.insert(
                            system_i.lock()->m_func_addr
                        );
                    }
                }
            }
        }
    }
    for (auto& [addr, system] : systems) {
        if (system->m_schedule.m_stage_type_hash == m_stage_type_hash &&
            system->m_schedule.m_stage_value == m_stage_value) {
            m_systems.push_back(system);
            for (auto before_ptr : system->m_system_ptrs_before) {
                if (systems.find(before_ptr) != systems.end()) {
                    auto sys_before = systems.find(before_ptr)->second;
                    if (sys_before->m_schedule.m_stage_type_hash ==
                            m_stage_type_hash &&
                        sys_before->m_schedule.m_stage_value == m_stage_value) {
                        system->m_systems_before.insert(sys_before);
                        sys_before->m_systems_after.insert(system);
                        sys_before->m_system_ptrs_after.insert(addr);
                    }
                }
            }
            for (auto after_ptr : system->m_system_ptrs_after) {
                if (systems.find(after_ptr) != systems.end()) {
                    auto sys_after = systems.find(after_ptr)->second;
                    if (sys_after->m_schedule.m_stage_type_hash ==
                            m_stage_type_hash &&
                        sys_after->m_schedule.m_stage_value == m_stage_value) {
                        system->m_systems_after.insert(sys_after);
                        sys_after->m_systems_before.insert(system);
                        sys_after->m_system_ptrs_before.insert(addr);
                    }
                }
            }
        }
    }
    if (build_logger->level() == spdlog::level::debug)
        for (auto& [addr, system] : systems) {
            if (system->m_schedule.m_stage_type_hash == m_stage_type_hash &&
                system->m_schedule.m_stage_value == m_stage_value)
                build_logger->debug(
                    "    System {:#016x} has {:d} defined prevs.", (size_t)addr,
                    system->m_systems_before.size()
                );
        }
    build_logger->debug(
        "> Stage runner {} : {:d} built. With {} systems in.",
        m_stage_type_hash->name(), m_stage_value, m_systems.size()
    );
}

void SubStageRunner::prepare() {
    std::erase_if(m_systems, [](auto system) { return system.expired(); });
    for (auto system : m_systems) {
        if (auto system_ptr = system.lock()) {
            system_ptr->reset_reach_time();
            system_ptr->clear_temp();
        }
    }
    std::sort(m_systems.begin(), m_systems.end(), [](auto a, auto b) {
        auto a_ptr = a.lock();
        auto b_ptr = b.lock();
        return a_ptr->reach_time() < b_ptr->reach_time();
    });
    for (size_t i = 0; i < m_systems.size(); i++) {
        for (size_t j = i + 1; j < m_systems.size(); j++) {
            if (auto system_ptr = m_systems[i].lock()) {
                if (auto system_ptr2 = m_systems[j].lock()) {
                    if (system_ptr->m_system->contrary_to(system_ptr2->m_system
                        )) {
                        system_ptr->m_temp_after.insert(system_ptr2);
                        system_ptr2->m_temp_before.insert(system_ptr);
                    }
                }
            }
        }
    }
    m_head->clear_temp();
    for (auto system : m_systems) {
        if (auto system_ptr = system.lock()) {
            if (system_ptr->m_system_ptrs_before.empty() &&
                system_ptr->m_temp_before.empty()) {
                m_head->m_temp_after.insert(system);
                system_ptr->m_temp_before.insert(m_head);
            }
        }
    }
    for (auto system : m_systems) {
        if (auto system_ptr = system.lock()) {
            system_ptr->m_prev_cout = system_ptr->m_temp_before.size() +
                                      system_ptr->m_systems_before.size();
        }
    }
}

void SubStageRunner::notify(std::weak_ptr<SystemNode> system) {
    m_msg_queue.push(system);
}

void SubStageRunner::run(std::weak_ptr<SystemNode> system) {
    if (auto system_ptr = system.lock()) {
        auto iter = (*m_workers).find(system_ptr->m_worker_name);
        if (iter == (*m_workers).end()) {
            run_logger->warn(
                "Worker name : {} is not a valid name, which is used by system "
                "{:#016x}",
                system_ptr->m_worker_name, (size_t)system_ptr->m_func_addr
            );
            notify(system);
            return;
        }
        auto worker = iter->second;
        auto ftr = worker->submit_task([=] {
            if (system_ptr->m_system &&
                system_ptr->condition_pass(m_src.get(), m_dst.get())) {
                system_ptr->run(m_src.get(), m_dst.get());
            }
            notify(system);
        });
    }
}

void SubStageRunner::run() {
    size_t m_unfinished_system_count = m_systems.size() + 1;
    size_t m_running_system_count = 0;
    run(m_head);
    m_running_system_count++;
    while (m_unfinished_system_count > 0) {
        if (m_running_system_count == 0) {
            run_logger->warn(
                "Deadlock detected, skipping remaining systems at stage {} "
                ": {:d}. With {:d} systems remaining.",
                m_stage_type_hash->name(), m_stage_value,
                m_unfinished_system_count
            );
            break;
        }
        m_msg_queue.wait();
        auto system = m_msg_queue.front();
        m_unfinished_system_count--;
        m_running_system_count--;
        if (auto system_ptr = system.lock()) {
            for (auto& next : system_ptr->m_systems_after) {
                if (auto next_ptr = next.lock()) {
                    next_ptr->m_prev_cout--;
                    if (next_ptr->m_prev_cout == 0) {
                        run(next);
                        m_running_system_count++;
                    }
                }
            }
            for (auto& next : system_ptr->m_temp_after) {
                if (auto next_ptr = next.lock()) {
                    next_ptr->m_prev_cout--;
                    if (next_ptr->m_prev_cout == 0) {
                        run(next);
                        m_running_system_count++;
                    }
                }
            }
        }
        m_msg_queue.pop();
    }
}

StageRunner::StageRunner(
    std::shared_ptr<SubApp> src,
    std::shared_ptr<SubApp> dst,
    std::unordered_map<const type_info*, std::vector<size_t>>* sets,
    std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>* workers
)
    : m_src(src), m_dst(dst), m_sets(sets), m_workers(workers) {}

void StageRunner::build(
    const std::unordered_map<void*, std::shared_ptr<SystemNode>>& systems
) {
    for (auto& stage_runner : m_sub_stage_runners) {
        stage_runner.build(systems);
    }
}

void StageRunner::prepare() {
    for (auto& stage_runner : m_sub_stage_runners) {
        stage_runner.prepare();
    }
}

void StageRunner::run() {
    for (auto& stage_runner : m_sub_stage_runners) {
        stage_runner.run();
    }
}

Runner::Runner(
    std::unordered_map<const type_info*, std::shared_ptr<SubApp>>* sub_apps,
    std::vector<std::pair<std::string, size_t>> workers
)
    : m_sub_apps(sub_apps) {
    for (auto worker : workers) {
        add_worker(worker.first, worker.second);
    }
}
void Runner::add_worker(std::string name, int num_threads) {
    m_workers.insert({name, std::make_shared<BS::thread_pool>(num_threads)});
}
std::shared_ptr<StageRunnerInfo> Runner::prepare(
    std::vector<std::shared_ptr<StageRunnerInfo>>& stages
) {
    for (auto& stage : stages) {
        stage->reset();
    }
    auto head = std::make_shared<StageRunnerInfo>();
    std::sort(stages.begin(), stages.end(), [](auto a, auto b) {
        return a->depth() < b->depth();
    });
    for (size_t i = 0; i < stages.size(); i++) {
        for (size_t j = i + 1; j < stages.size(); j++) {
            auto stage_i = stages[i];
            auto stage_j = stages[j];
            if (stage_i->depth() == stage_j->depth()) {
                if (stage_i->conflict(stage_j)) {
                    stage_i->m_after_temp.insert(stage_j);
                    stage_j->m_before_temp.insert(stage_i);
                }
            }
        }
    }
    for (auto& stage : stages) {
        if (stage->depth() == 0) {
            head->m_after_temp.insert(stage);
            stage->m_before_temp.insert(head);
        } else {
            break;
        }
    }
    for (auto& stage : stages) {
        stage->before_count =
            stage->m_before_temp.size() + stage->m_before.size();
    }
    return head;
}
void Runner::run(std::shared_ptr<StageRunnerInfo> stage) {
    auto ftr = m_runner_pool.submit_task([=] {
        auto ptr = stage->operator->();
        if (ptr) {
            ptr->prepare();
            ptr->run();
        }
        m_msg_queue.push(stage);
    });
}
void Runner::run(
    std::vector<std::shared_ptr<StageRunnerInfo>>& stages,
    std::shared_ptr<StageRunnerInfo> head
) {
    size_t m_unfinished_stage_count = stages.size() + 1;
    size_t m_running_stage_count = 0;
    run(head);
    m_running_stage_count++;
    while (m_unfinished_stage_count > 0) {
        if (m_running_stage_count == 0) {
            run_logger->warn("Deadlock detected, skipping remaining stages.");
            break;
        }
        m_msg_queue.wait();
        auto stage = m_msg_queue.front();
        m_unfinished_stage_count--;
        m_running_stage_count--;
        for (auto& nextr : stage->m_after_temp) {
            auto next = nextr.lock();
            next->before_count--;
            if (next->before_count == 0) {
                run(next);
                m_running_stage_count++;
            }
        }
        for (auto& nextr : stage->m_after) {
            auto next = nextr.lock();
            next->before_count--;
            if (next->before_count == 0) {
                run(next);
                m_running_stage_count++;
            }
        }
        m_msg_queue.pop();
    }
}
void Runner::run_startup() {
    auto head = prepare(m_startup_stages_vec);
    run(m_startup_stages_vec, head);
}
void Runner::run_transition() {
    auto head = prepare(m_transition_stages_vec);
    run(m_transition_stages_vec, head);
}
void Runner::run_loop() {
    auto head = prepare(m_loop_stages_vec);
    run(m_loop_stages_vec, head);
}
void Runner::run_exit() {
    auto head = prepare(m_exit_stages_vec);
    run(m_exit_stages_vec, head);
}
void Runner::build() {
    for (auto& [ptr, system] : m_systems) {
        system->m_systems_after.clear();
        system->m_systems_before.clear();
    }
    for (auto& [stage, stage_runner] : m_startup_stages) {
        stage_runner->operator->()->build(m_systems);
    }
    for (auto& [stage, stage_runner] : m_loop_stages) {
        stage_runner->operator->()->build(m_systems);
    }
    for (auto& [stage, stage_runner] : m_exit_stages) {
        stage_runner->operator->()->build(m_systems);
    }
}
void Runner::end_commands() {
    for (auto& [_, sub_app] : *m_sub_apps) {
        sub_app->end_commands();
    }
}

void Runner::removing_system(void* func) {
    if (m_systems.find(func) != m_systems.end()) {
        m_systems.erase(func);
    } else {
        setup_logger->warn(
            "Trying to remove system {:#016x}, which does not exist.",
            (size_t)func
        );
    }
}

Worker::Worker() : name("default") {}
Worker::Worker(std::string str) : name(str) {}

bool pixel_engine::app::check_exit(app::EventReader<AppExit> reader) {
    if (!reader.empty()) run_logger->info("Exit event received.");
    return !reader.empty();
}

void App::PluginCache::add_system(void* system_ptr) {
    m_system_ptrs.insert(system_ptr);
}

App::AddSystemReturn::AddSystemReturn(App* app, SystemNode* sys)
    : m_app(app), m_system(sys) {}
App* App::AddSystemReturn::operator->() { return m_app; }
App::AddSystemReturn& App::AddSystemReturn::use_worker(std::string name) {
    if (m_system) {
        m_system->m_worker_name = name;
    }
    return *this;
}
App::AddSystemReturn& App::AddSystemReturn::run_if(
    std::shared_ptr<BasicSystem<bool>> cond
) {
    m_system->m_conditions.insert(cond);
    return *this;
}

void App::update_states() {
    run_logger->debug("Updating states.");
    for (auto& [_, sub_app] : m_sub_apps) {
        sub_app->update_states();
    }
    m_runner.m_workers["default"]->wait();
}

App::App() {
    add_sub_app<MainSubApp>();
    add_sub_app<RenderSubApp>();
    m_main_sub_app = &typeid(MainSubApp);
    m_runner.assign_startup_stage<MainSubApp, MainSubApp>(
        StartStage::PreStartup, StartStage::Startup, StartStage::PostStartup
    );
    m_runner.assign_transition_stage<MainSubApp, MainSubApp>(
        StateTransitionStage::StateTransit
    );
    m_runner.assign_loop_stage<MainSubApp, MainSubApp>(
        LoopStage::First, LoopStage::PreUpdate, LoopStage::Update,
        LoopStage::PostUpdate, LoopStage::Last
    );
    m_runner
        .assign_loop_stage<MainSubApp, MainSubApp>(
            RenderStage::Prepare, RenderStage::PreRender, RenderStage::Render,
            RenderStage::PostRender
        )
        .after<LoopStage>();
    m_runner.assign_exit_stage<MainSubApp, MainSubApp>(
        ExitStage::PreShutdown, ExitStage::Shutdown, ExitStage::PostShutdown
    );
}

App* App::operator->() { return this; }

App& App::log_level(Loggers logger, spdlog::level::level_enum level) {
    switch (logger) {
        case Setup:
            setup_logger->set_level(level);
            break;

        case Build:
            build_logger->set_level(level);
            break;

        case Run:
            run_logger->set_level(level);
            break;

        default:
            break;
    }
    return *this;
}

App& App::log_level(spdlog::level::level_enum level) {
    setup_logger->set_level(level);
    build_logger->set_level(level);
    run_logger->set_level(level);
    return *this;
}

App& App::enable_loop() {
    m_loop_enabled = true;
    return *this;
}

void App::build_plugins() {
    for (auto& [plugin_type, plugin] : m_plugins) {
        m_building_plugin_type = plugin_type;
        plugin->build(*this);
        for (auto& [_, sub_app] : m_sub_apps) {
            sub_app->emplace_resource(plugin_type, plugin);
        }
        m_building_plugin_type = nullptr;
    }
}

void App::run() {
    build_plugins();
    auto& main_sub_app = m_sub_apps[m_main_sub_app];
    build_logger->info("Building app.");
    m_runner.build();
    build_logger->info("Building done.");
    run_logger->info("Running app.");
    m_runner.run_startup();
    run_logger->debug("Running loop.");
    do {
        run_logger->debug("Run Loop systems.");
        m_runner.run_loop();
        run_logger->debug("Run state transition systems.");
        m_runner.run_transition();
        update_states();
        run_logger->debug("Tick events.");
        for (auto& [_, sub_app] : m_sub_apps) {
            sub_app->tick_events();
        }
    } while (m_loop_enabled && !main_sub_app->run_system_v(check_exit));
    run_logger->info("Exiting app.");
    m_runner.run_exit();
    run_logger->info("App terminated.");
}

EntityCommand::EntityCommand(
    entt::registry* registry,
    entt::entity entity,
    std::unordered_map<entt::entity, std::unordered_set<entt::entity>>*
        entity_tree,
    std::shared_ptr<std::vector<entt::entity>> despawns,
    std::shared_ptr<std::vector<entt::entity>> recursive_despawns
)
    : m_registry(registry),
      m_entity(entity),
      m_entity_tree(entity_tree),
      m_despawns(despawns),
      m_recursive_despawns(recursive_despawns) {}

void EntityCommand::despawn() { m_despawns->push_back(m_entity); }

void EntityCommand::despawn_recurse() {
    m_recursive_despawns->push_back(m_entity);
}

Command::Command(
    entt::registry* registry,
    std::unordered_map<const type_info*, std::shared_ptr<void>>* resources,
    std::unordered_map<entt::entity, std::unordered_set<entt::entity>>*
        entity_tree
)
    : m_registry(registry), m_resources(resources), m_entity_tree(entity_tree) {
    m_despawns = std::make_shared<std::vector<entt::entity>>();
    m_recursive_despawns = std::make_shared<std::vector<entt::entity>>();
}

EntityCommand Command::entity(entt::entity entity) {
    return EntityCommand(
        m_registry, entity, m_entity_tree, m_despawns, m_recursive_despawns
    );
}

void Command::end() {
    for (auto entity : *m_despawns) {
        m_registry->destroy(entity);
        for (auto child : (*m_entity_tree)[entity]) {
            m_registry->erase<Parent>(child);
        }
    }
    for (auto entity : *m_recursive_despawns) {
        m_registry->destroy(entity);
        for (auto child : (*m_entity_tree)[entity]) {
            m_registry->destroy(child);
        }
    }
}

Command SubApp::command() {
    return Command(
        &m_world.m_registry, &m_world.m_resources, &m_world.m_entity_tree
    );
}

void SubApp::tick_events() {
    for (auto& [key, events] : m_world.m_events) {
        while (!events->empty()) {
            auto& evt = events->front();
            if (evt.ticks >= 1) {
                events->pop_front();
            } else {
                break;
            }
        }
        for (auto& evt : *events) {
            evt.ticks++;
        }
    }
}

void SubApp::update_states() {
    for (auto& update : m_state_updates) {
        update->run(this, this);
    }
}

void SubApp::end_commands() {
    for (auto& cmd : m_commands) {
        cmd.end();
    }
    m_commands.clear();
}