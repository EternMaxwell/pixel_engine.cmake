#pragma once

#include <spdlog/spdlog.h>

#include <BS_thread_pool.hpp>
#include <entt/entity/registry.hpp>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "command.h"
#include "event.h"
#include "query.h"
#include "resource.h"
#include "system.h"
#include "tools.h"
#include "world.h"

namespace pixel_engine {
namespace app {

std::shared_ptr<spdlog::logger> logger = spdlog::default_logger()->clone("app");

enum BasicStage { Start, StateTransition, Loop, Exit };

enum StartStage { PreStartup, Startup, PostStartup };

enum StateTransitionStage { StateTransit };

enum LoopStage { First, PreUpdate, Update, PostUpdate, Last };

enum RenderStage { Prepare, PreRender, Render, PostRender };

enum ExitStage { PreShutdown, Shutdown, PostShutdown };

struct SystemStage {
    const type_info* m_stage_type_hash;
    size_t m_stage_value;
    template <typename T>
    SystemStage(T t) {
        m_stage_type_hash = &typeid(T);
        m_stage_value = static_cast<size_t>(t);
    }
};

struct SystemSet {
    const type_info* m_set_type_hash;
    size_t m_set_value;
    template <typename T>
    SystemSet(T t) {
        m_set_type_hash = &typeid(T);
        m_set_value = static_cast<size_t>(t);
    }
};

struct Schedule {
    const type_info* m_stage_type_hash;
    size_t m_stage_value;
    std::shared_ptr<BasicSystem<bool>> m_condition;
    Schedule() {}
    template <typename T>
    Schedule(T t) {
        m_stage_type_hash = &typeid(T);
        m_stage_value = static_cast<size_t>(t);
    }
    template <typename T, typename... Args>
    Schedule(T t, Args... args) {
        m_stage_type_hash = &typeid(T);
        m_stage_value = static_cast<size_t>(t);
        m_condition = std::make_shared<BasicSystem<bool>>(nullptr, args...);
    }
};

struct SetMap {
    std::unordered_map<const type_info*, std::vector<size_t>> m_sets;
    template <typename T, typename... Args>
    void configure_sets(T t, Args... args) {
        std::vector<size_t> set = {
            static_cast<size_t>(t), static_cast<size_t>(args)...};
        m_sets[&typeid(T)] = set;
    }
};

struct SystemNode {
    Schedule m_schedule;
    std::vector<SystemSet> m_in_sets;
    void* m_func_addr;
    std::string m_worker_name = "default";
    std::shared_ptr<BasicSystem<void>> m_system;
    std::unordered_set<void*> m_system_ptrs_before;
    std::unordered_set<void*> m_system_ptrs_after;
    std::unordered_set<std::weak_ptr<SystemNode>> m_systems_before;
    std::unordered_set<std::weak_ptr<SystemNode>> m_systems_after;
    std::unordered_set<std::weak_ptr<SystemNode>> m_temp_before;
    std::unordered_set<std::weak_ptr<SystemNode>> m_temp_after;
    std::optional<double> m_reach_time;

    size_t m_prev_cout;
    size_t m_next_cout;

    SystemNode() {}
    template <typename... Args>
    SystemNode(Schedule schedule, void (*func)(Args...)) {
        m_schedule = schedule;
        m_system = std::make_shared<System<Args...>>(func);
        m_func_addr = func;
    }

    double reach_time() {
        if (m_reach_time.has_value()) {
            return m_reach_time.value();
        } else {
            double max_time = 0.0;
            for (auto system : m_systems_before) {
                if (auto system_ptr = system.lock()) {
                    max_time = std::max(
                        max_time, system_ptr->reach_time() +
                                      system_ptr->m_system->get_avg_time());
                }
            }
            m_reach_time = max_time;
            return max_time;
        }
    }

    void reset_reach_time() { m_reach_time.reset(); }

    void clear_temp() {
        m_temp_before.clear();
        m_temp_after.clear();
    }
};

struct MsgQueue {
    std::queue<std::weak_ptr<SystemNode>> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    MsgQueue() {}
    MsgQueue(const MsgQueue& other) : m_queue(other.m_queue) {}
    auto& operator=(const MsgQueue& other) {
        m_queue = other.m_queue;
        return *this;
    }
    void push(std::weak_ptr<SystemNode> system) {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_queue.push(system);
        m_cv.notify_one();
    }
    std::weak_ptr<SystemNode> pop() {
        std::unique_lock<std::mutex> lk(m_mutex);
        auto system = m_queue.front();
        m_queue.pop();
        return system;
    }
    std::weak_ptr<SystemNode> front() {
        std::unique_lock<std::mutex> lk(m_mutex);
        return m_queue.front();
    }
    bool empty() {
        std::unique_lock<std::mutex> lk(m_mutex);
        return m_queue.empty();
    }
    void wait() {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cv.wait(lk, [&] { return !m_queue.empty(); });
    }
};

struct StageRunner {
   private:
    World* m_world;
    const type_info* m_stage_type_hash;
    size_t m_stage_value;
    std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
        m_workers;
    std::unordered_map<const type_info*, std::vector<size_t>>* m_sets;
    std::vector<std::weak_ptr<SystemNode>> m_systems;

    std::shared_ptr<SystemNode> m_head = std::make_shared<SystemNode>();

    MsgQueue m_msg_queue;

   public:
    template <typename StageT>
    StageRunner(
        World* world, StageT stage,
        std::unordered_map<const type_info*, std::vector<size_t>>* sets,
        std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
            workers)
        : m_world(world),
          m_stage_type_hash(&typeid(StageT)),
          m_stage_value(static_cast<size_t>(stage)),
          m_sets(sets),
          m_workers(workers) {}

    void build(
        const std::unordered_map<void*, std::shared_ptr<SystemNode>>& systems) {
        logger->debug(
            "Building stage {} : {:d}.", m_stage_type_hash->name(),
            m_stage_value);
        m_systems.clear();
        for (auto& [id, sets] : *m_sets) {
            std::vector<std::vector<std::weak_ptr<SystemNode>>> set_systems;
            for (auto set : sets) {
                std::vector<std::weak_ptr<SystemNode>> ss;
                set_systems.push_back(ss);
                auto& set_system = set_systems.back();
                for (auto& [addr, system] : systems) {
                    if (system->m_schedule.m_stage_type_hash ==
                            m_stage_type_hash &&
                        system->m_schedule.m_stage_value == m_stage_value &&
                        std::find_if(
                            system->m_in_sets.begin(), system->m_in_sets.end(),
                            [=](const auto& sset) {
                                return sset.m_set_type_hash == id &&
                                       sset.m_set_value == set;
                            }) != system->m_in_sets.end()) {
                        set_system.push_back(system);
                    }
                }
            }
            for (size_t i = 0; i < set_systems.size(); i++) {
                for (size_t j = i + 1; j < set_systems.size(); j++) {
                    for (auto& system_i : set_systems[i]) {
                        for (auto& system_j : set_systems[j]) {
                            system_i.lock()->m_system_ptrs_after.insert(
                                system_j.lock()->m_func_addr);
                            system_j.lock()->m_system_ptrs_before.insert(
                                system_i.lock()->m_func_addr);
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
                            sys_before->m_schedule.m_stage_value ==
                                m_stage_value) {
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
                            sys_after->m_schedule.m_stage_value ==
                                m_stage_value) {
                            system->m_systems_after.insert(sys_after);
                            sys_after->m_systems_before.insert(system);
                            sys_after->m_system_ptrs_before.insert(addr);
                        }
                    }
                }
            }
        }
        if (logger->level() == spdlog::level::debug)
            for (auto& [addr, system] : systems) {
                if (system->m_schedule.m_stage_type_hash == m_stage_type_hash &&
                    system->m_schedule.m_stage_value == m_stage_value)
                    logger->debug(
                        "    System {:#016x} has {:d} defined prevs.",
                        (size_t)addr, system->m_systems_before.size());
            }
        logger->debug(
            "> Stage runner {} : {:d} built. With {} systems in.",
            m_stage_type_hash->name(), m_stage_value, m_systems.size());
    }

    void prepare() {
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
                        if (system_ptr->m_system->contrary_to(
                                system_ptr2->m_system)) {
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

    void notify(std::weak_ptr<SystemNode> system) { m_msg_queue.push(system); }

    void run(std::weak_ptr<SystemNode> system) {
        if (auto system_ptr = system.lock()) {
            auto worker = (*m_workers)[system_ptr->m_worker_name];
            auto ftr = worker->submit_task([=] {
                if (system_ptr->m_system &&
                    (!system_ptr->m_schedule.m_condition ||
                     system_ptr->m_schedule.m_condition->run(m_world))) {
                    system_ptr->m_system->run(m_world);
                }
                notify(system);
            });
        }
    }

    void run() {
        size_t m_unfinished_system_count = m_systems.size() + 1;
        size_t m_running_system_count = 0;
        run(m_head);
        m_running_system_count++;
        while (m_unfinished_system_count > 0) {
            if (m_running_system_count == 0) {
                logger->warn(
                    "Deadlock detected, skipping remaining systems at stage {} "
                    ": {:d}.",
                    m_stage_type_hash->name(), m_stage_value);
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
};

struct after {
    std::vector<void*> ptrs;
    template <typename... Args>
    after(Args... args) {
        ptrs = {((void*)args)...};
    }
};

struct before {
    std::vector<void*> ptrs;
    template <typename... Args>
    before(Args... args) {
        ptrs = {((void*)args)...};
    }
};

struct in_set {
    std::vector<SystemSet> sets;
    template <typename... Args>
    in_set(Args... args) {
        sets = {(SystemSet(args))...};
    }
};

struct Runner {
    World* m_world;
    std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>> m_workers;
    std::unordered_map<BasicStage, std::vector<StageRunner>> m_stages;
    std::unordered_map<void*, std::shared_ptr<SystemNode>> m_systems;
    std::unordered_map<const type_info*, std::vector<size_t>> m_sets;
    void add_worker(std::string name, int num_threads) {
        m_workers.insert(
            {name, std::make_shared<BS::thread_pool>(num_threads)});
    }
    Runner(std::vector<std::pair<std::string, size_t>> workers) {
        for (auto worker : workers) {
            add_worker(worker.first, worker.second);
        }
    }
    template <typename... SubStages>
    void configure_stage(BasicStage stage, SubStages... stages) {
        std::vector<StageRunner> stage_runners = {
            StageRunner(m_world, stages, &m_sets, &m_workers)...};
        m_stages[stage] = stage_runners;
    }
    template <typename T, typename... Sets>
    void configure_sets(T set, Sets... sets) {
        m_sets[&typeid(T)] = {
            static_cast<size_t>(set), static_cast<size_t>(sets)...};
    }
    void run_stage(BasicStage stage) {
        for (auto& stage_runner : m_stages[stage]) {
            stage_runner.prepare();
            stage_runner.run();
        }
    }
    void build() {
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

    template <typename... Args>
    std::shared_ptr<SystemNode> add_system(
        Schedule schedule, void (*func)(Args...),
        std::string worker_name = "default", after befores = after(),
        before afters = before(), in_set sets = in_set()) {
        if (m_systems.find(func) != m_systems.end()) {
            logger->warn(
                "Trying to add system {:#016x} : {}, which already exists.",
                (size_t)func, typeid(func).name());
            return nullptr;
        }
        std::shared_ptr<SystemNode> node =
            std::make_shared<SystemNode>(schedule, func);
        node->m_in_sets = sets.sets;
        node->m_worker_name = worker_name;
        node->m_system_ptrs_before.insert(
            befores.ptrs.begin(), befores.ptrs.end());
        node->m_system_ptrs_after.insert(
            afters.ptrs.begin(), afters.ptrs.end());
        m_systems.insert({(void*)func, node});
        return node;
    }

    void removing_system(void* func) {
        if (m_systems.find(func) != m_systems.end()) {
            m_systems.erase(func);
        } else {
            logger->warn(
                "Trying to remove system {:#016x}, which does not exist.",
                (size_t)func);
        }
    }
};

struct Worker {
    std::string name;
    Worker() : name("default") {}
    Worker(std::string str) : name(str) {}
};

struct AppExit {};

bool check_exit(EventReader<AppExit> reader) { return !reader.empty(); }

struct App;

struct Plugin {
    virtual void build(App& app) = 0;
};

struct App {
    struct PluginCache {
       private:
        const type_info* m_plugin_type;
        std::unordered_set<void*> m_system_ptrs;

       public:
        PluginCache() {}
        template <typename T>
        PluginCache(const T&& t) {
            m_plugin_type = &typeid(T);
        }
        void add_system(void* system_ptr) { m_system_ptrs.insert(system_ptr); }
    };

    struct AddSystemReturn {
       private:
        std::shared_ptr<SystemNode> m_system;
        App* m_app;

       public:
        AddSystemReturn(App* app, std::weak_ptr<SystemNode> sys)
            : m_app(app), m_system(sys) {}
        App* operator->() { return m_app; }
        template <typename... Sets>
        AddSystemReturn& in_set(Sets... sets) {
            if (m_system) {
                app::in_set sets_container = app::in_set(sets...);
                for (auto& sys_set : sets_container.sets) {
                    m_system->m_in_sets.push_back(sys_set);
                }
            }
            return *this;
        }
        template <typename... Addrs>
        AddSystemReturn& before(Addrs... addrs) {
            if (m_system) {
                std::vector<void*> addrs_v = {((void*)addrs)...};
                for (auto ptr : addrs_v) {
                    m_system->m_system_ptrs_after.insert(ptr);
                }
            }
            return *this;
        }
        template <typename... Addrs>
        AddSystemReturn& after(Addrs... addrs) {
            if (m_system) {
                std::vector<void*> addrs_v = {((void*)addrs)...};
                for (auto ptr : addrs_v) {
                    m_system->m_system_ptrs_before.insert(ptr);
                }
            }
            return *this;
        }
        AddSystemReturn& use_worker(std::string name) {
            if (m_system) {
                m_system->m_worker_name = name;
            }
            return *this;
        }
    };

   private:
    World m_world;
    Runner m_runner = Runner({{"default", 8}, {"single", 1}});
    std::vector<std::shared_ptr<BasicSystem<void>>> m_state_update;
    std::unordered_map<const type_info*, PluginCache> m_plugin_caches;
    bool m_loop_enabled = false;
    const type_info* m_building_plugin_type = nullptr;

    template <typename T>
    auto state_update() {
        using namespace internal_components;
        return
            [&](Resource<State<T>> state, Resource<NextState<T>> state_next) {
                if (state.has_value() && state_next.has_value()) {
                    state->just_created = false;
                    state->m_state = state_next->m_state;
                }
            };
    }

    void update_states() {
        logger->debug("Updating states.");
        for (auto& update : m_state_update) {
            auto ftr = m_runner.m_workers["default"]->submit_task(
                [=] { update->run(&m_world); });
        }
        m_runner.m_workers["default"]->wait();
    }

   public:
    App() {
        m_runner.m_world = &m_world;
        m_runner.configure_stage(
            BasicStage::Start, StartStage::PreStartup, StartStage::Startup,
            StartStage::PostStartup);
        m_runner.configure_stage(
            BasicStage::StateTransition, StateTransitionStage::StateTransit);
        m_runner.configure_stage(
            BasicStage::Loop, LoopStage::First, LoopStage::PreUpdate,
            LoopStage::Update, LoopStage::PostUpdate, LoopStage::Last,
            RenderStage::Prepare, RenderStage::PreRender, RenderStage::Render,
            RenderStage::PostRender);
        m_runner.configure_stage(
            BasicStage::Exit, ExitStage::PreShutdown, ExitStage::Shutdown,
            ExitStage::PostShutdown);
    }

    App* operator->() { return this; }

    App& log_level(spdlog::level::level_enum level) {
        logger->set_level(level);
        return *this;
    }

    App& enable_loop() { m_loop_enabled = true; }

    template <typename T, typename... Args>
    App& configure_sets(T set, Args... sets) {
        m_runner.configure_sets(set, sets...);
        return *this;
    }

    template <typename... Args, typename... TupleT>
    AddSystemReturn add_system(
        Schedule schedule, void (*func)(Args...), TupleT... modifiers) {
        auto mod_tuple = std::make_tuple(modifiers...);
        auto ptr = m_runner.add_system(
            schedule, func, app_tools::tuple_get<Worker>(mod_tuple).name,
            app_tools::tuple_get<after>(mod_tuple),
            app_tools::tuple_get<before>(mod_tuple),
            app_tools::tuple_get<in_set>(mod_tuple));
        if (m_building_plugin_type && ptr) {
            m_plugin_caches[m_building_plugin_type].add_system((void*)func);
        }
        return AddSystemReturn(this, ptr);
    }
    template <typename... Args>
    AddSystemReturn add_system(Schedule schedule, void (*func)(Args...)) {
        auto ptr = m_runner.add_system(schedule, func);
        if (m_building_plugin_type && ptr) {
            m_plugin_caches[m_building_plugin_type].add_system((void*)func);
        }
        return AddSystemReturn(this, ptr);
    }

    template <typename T>
    App& add_plugin(const T&& plugin) {
        if (m_plugin_caches.find(&typeid(T)) != m_plugin_caches.end()) {
            logger->warn(
                "Trying to add plugin {}, which already exists.",
                typeid(T).name());
            return *this;
        }
        m_building_plugin_type = &typeid(T);
        m_plugin_caches.insert({m_building_plugin_type, PluginCache(plugin)});
        plugin.build(this);
        m_building_plugin_type = nullptr;
        return *this;
    }

    template <typename T>
    App& remove_plugin() {
        if (m_plugin_caches.find(&typeid(T)) != m_plugin_caches.end()) {
            for (auto system_ptr : m_plugin_caches[&typeid(T)].m_system_ptrs) {
                m_runner.removing_system(system_ptr);
            }
            m_plugin_caches.erase(&typeid(T));
        } else {
            logger->warn(
                "Trying to remove plugin {}, which does not exist.",
                typeid(T).name());
        }
        return *this;
    }

    void run() {
        logger->info("Building app.");
        m_runner.build();
        logger->info("Building done.");
        logger->info("Running app.");
        m_runner.run_stage(BasicStage::Start);
        logger->debug("Running loop.");
        do {
            logger->debug("Run Loop systems.");
            m_runner.run_stage(BasicStage::Loop);
            logger->debug("Run state transition systems.");
            m_runner.run_stage(BasicStage::StateTransition);
            update_states();
            logger->debug("Tick events.");
            m_world.tick_events();
        } while (m_loop_enabled && !m_world.run_system_v(check_exit));
        logger->info("Exiting app.");
        m_runner.run_stage(BasicStage::Exit);
    }

    template <typename T>
    App& insert_state(T state) {
        m_world.insert_state(state);
        m_state_update.push_back(
            std::make_unique<
                System<Resource<State<T>>, Resource<NextState<T>>>>(
                state_update<T>()));
        return *this;
    }

    template <typename T>
    App& init_state() {
        m_world.init_state<T>();
        m_state_update.push_back(
            std::make_unique<
                System<Resource<State<T>>, Resource<NextState<T>>>>(
                state_update<T>()));
        return *this;
    }
};
}  // namespace app
}  // namespace pixel_engine