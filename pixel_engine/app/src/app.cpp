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

bool SystemNode::condition_pass(World* world) {
    bool pass = true;
    if (m_schedule.m_condition) {
        pass &= m_schedule.m_condition->run(world);
    }
    if (!pass) return false;
    for (auto& cond : m_conditions) {
        pass &= cond->run(world);
        if (!pass) return false;
    }
    return true;
}
void SystemNode::run(World* world) {
    if (m_system) m_system->run(world);
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
MsgQueue::MsgQueue(const MsgQueue& other) : m_queue(other.m_queue) {}
auto& MsgQueue::operator=(const MsgQueue& other) {
    m_queue = other.m_queue;
    return *this;
}
void MsgQueue::push(std::weak_ptr<SystemNode> system) {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_queue.push(system);
    m_cv.notify_one();
}
std::weak_ptr<SystemNode> MsgQueue::pop() {
    std::unique_lock<std::mutex> lk(m_mutex);
    auto system = m_queue.front();
    m_queue.pop();
    return system;
}
std::weak_ptr<SystemNode> MsgQueue::front() {
    std::unique_lock<std::mutex> lk(m_mutex);
    return m_queue.front();
}
bool MsgQueue::empty() {
    std::unique_lock<std::mutex> lk(m_mutex);
    return m_queue.empty();
}
void MsgQueue::wait() {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cv.wait(lk, [&] { return !m_queue.empty(); });
}

void StageRunner::build(
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

void StageRunner::prepare() {
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

void StageRunner::notify(std::weak_ptr<SystemNode> system) {
    m_msg_queue.push(system);
}

void StageRunner::run(std::weak_ptr<SystemNode> system) {
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
            if (system_ptr->m_system && system_ptr->condition_pass(m_world)) {
                system_ptr->run(m_world);
            }
            notify(system);
        });
    }
}

void StageRunner::run() {
    size_t m_unfinished_system_count = m_systems.size() + 1;
    size_t m_running_system_count = 0;
    run(m_head);
    m_running_system_count++;
    while (m_unfinished_system_count > 0) {
        if (m_running_system_count == 0) {
            run_logger->warn(
                "Deadlock detected, skipping remaining systems at stage {} "
                ": {:d}.",
                m_stage_type_hash->name(), m_stage_value
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

Runner::Runner(std::vector<std::pair<std::string, size_t>> workers) {
    for (auto worker : workers) {
        add_worker(worker.first, worker.second);
    }
}
void Runner::add_worker(std::string name, int num_threads) {
    m_workers.insert({name, std::make_shared<BS::thread_pool>(num_threads)});
}
void Runner::run_stage(BasicStage stage) {
    for (auto& stage_runner : m_stages[stage]) {
        stage_runner.prepare();
        stage_runner.run();
    }
    m_world->end_commands();
}
void Runner::build() {
    for (auto& [ptr, system] : m_systems) {
        system->m_systems_after.clear();
        system->m_systems_before.clear();
    }
    for (auto& [stage, stage_runners] : m_stages) {
        for (auto& stage_runner : stage_runners) {
            stage_runner.build(m_systems);
        }
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

bool app::check_exit(app::EventReader<AppExit> reader) {
    if (!reader.empty()) run_logger->info("Exit event received.");
    return !reader.empty();
}

void App::PluginCache::add_system(void* system_ptr) {
    m_system_ptrs.insert(system_ptr);
}

App::AddSystemReturn::AddSystemReturn(App* app, std::weak_ptr<SystemNode> sys)
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

App::AddSystemReturn& App::AddSystemReturn::get_node(
    std::shared_ptr<SystemNode>& node
) {
    node = m_system;
    return *this;
}

void App::update_states() {
    run_logger->debug("Updating states.");
    for (auto& update : m_state_update) {
        auto ftr = m_runner.m_workers["default"]->submit_task([=] {
            update->run(&m_world);
        });
    }
    m_runner.m_workers["default"]->wait();
}

App::App() {
    m_runner.m_world = &m_world;
    m_runner.configure_stage(
        BasicStage::Start, StartStage::PreStartup, StartStage::Startup,
        StartStage::PostStartup
    );
    m_runner.configure_stage(
        BasicStage::StateTransition, StateTransitionStage::StateTransit
    );
    m_runner.configure_stage(
        BasicStage::Loop, LoopStage::First, LoopStage::PreUpdate,
        LoopStage::Update, LoopStage::PostUpdate, LoopStage::Last,
        RenderStage::Prepare, RenderStage::PreRender, RenderStage::Render,
        RenderStage::PostRender
    );
    m_runner.configure_stage(
        BasicStage::Exit, ExitStage::PreShutdown, ExitStage::Shutdown,
        ExitStage::PostShutdown
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

void App::run() {
    build_logger->info("Building app.");
    m_runner.build();
    build_logger->info("Building done.");
    run_logger->info("Running app.");
    m_runner.run_stage(BasicStage::Start);
    run_logger->debug("Running loop.");
    do {
        run_logger->debug("Run Loop systems.");
        m_runner.run_stage(BasicStage::Loop);
        run_logger->debug("Run state transition systems.");
        m_runner.run_stage(BasicStage::StateTransition);
        update_states();
        run_logger->debug("Tick events.");
        m_world.tick_events();
    } while (m_loop_enabled && !m_world.run_system_v(check_exit));
    run_logger->info("Exiting app.");
    m_runner.run_stage(BasicStage::Exit);
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
    std::unordered_map<size_t, std::shared_ptr<void>>* resources,
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

Command World::command() {
    return Command(&m_registry, &m_resources, &m_entity_tree);
}

void World::tick_events() {
    for (auto& [key, events] : m_events) {
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

void World::end_commands() {
    for (auto& cmd : m_commands) {
        cmd.end();
    }
    m_commands.clear();
}