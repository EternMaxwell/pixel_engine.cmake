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

static std::shared_ptr<spdlog::logger> logger =
    spdlog::default_logger()->clone("app");

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
    Schedule() = default;
    template <typename T>
    Schedule(T t) {
        m_stage_type_hash = &typeid(T);
        m_stage_value = static_cast<size_t>(t);
    }
    template <typename T, typename... Args>
    Schedule(T t, std::function<bool(Args...)> func) {
        m_stage_type_hash = &typeid(T);
        m_stage_value = static_cast<size_t>(t);
        m_condition = std::make_shared<Condition<Args...>>(func);
    }
};

struct SetMap {
    std::unordered_map<const type_info*, std::vector<size_t>> m_sets;
    template <typename T, typename... Args>
    void configure_sets(T t, Args... args) {
        std::vector<size_t> set = {
            static_cast<size_t>(t), static_cast<size_t>(args)...
        };
        m_sets[&typeid(T)] = set;
    }
};

struct SystemNode {
    Schedule m_schedule;
    std::unordered_set<std::shared_ptr<BasicSystem<bool>>> m_conditions;
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

    SystemNode() = default;
    template <typename... Args>
    SystemNode(Schedule schedule, void (*func)(Args...)) {
        m_schedule = schedule;
        m_system = std::make_shared<System<Args...>>(func);
        m_func_addr = func;
    }

    bool condition_pass(World* world);

    void run(World* world);

    double reach_time();

    void reset_reach_time();

    void clear_temp();
};

struct MsgQueue {
    std::queue<std::weak_ptr<SystemNode>> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    MsgQueue() = default;
    MsgQueue(const MsgQueue& other);
    auto& operator=(const MsgQueue& other);
    void push(std::weak_ptr<SystemNode> system);
    std::weak_ptr<SystemNode> pop();
    std::weak_ptr<SystemNode> front();
    bool empty();
    void wait();
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
        World* world,
        StageT stage,
        std::unordered_map<const type_info*, std::vector<size_t>>* sets,
        std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
            workers
    )
        : m_world(world),
          m_stage_type_hash(&typeid(StageT)),
          m_stage_value(static_cast<size_t>(stage)),
          m_sets(sets),
          m_workers(workers) {}
    void build(
        const std::unordered_map<void*, std::shared_ptr<SystemNode>>& systems
    );
    void prepare();
    void notify(std::weak_ptr<SystemNode> system);
    void run(std::weak_ptr<SystemNode> system);
    void run();
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
    template <typename... SubStages>
    void configure_stage(BasicStage stage, SubStages... stages) {
        std::vector<StageRunner> stage_runners = {
            StageRunner(m_world, stages, &m_sets, &m_workers)...
        };
        m_stages[stage] = stage_runners;
    }
    template <typename T, typename... Sets>
    void configure_sets(T set, Sets... sets) {
        m_sets[&typeid(T)] = {
            static_cast<size_t>(set), static_cast<size_t>(sets)...
        };
    }
    Runner(std::vector<std::pair<std::string, size_t>> workers);
    void add_worker(std::string name, int num_threads);
    void run_stage(BasicStage stage);
    void build();
    void removing_system(void* func);

    template <typename... Args>
    std::shared_ptr<SystemNode> add_system(
        Schedule schedule,
        void (*func)(Args...),
        std::string worker_name = "default",
        after befores = after(),
        before afters = before(),
        in_set sets = in_set()
    ) {
        if (m_systems.find(func) != m_systems.end()) {
            logger->warn(
                "Trying to add system {:#016x} : {}, which already exists.",
                (size_t)func, typeid(func).name()
            );
            return nullptr;
        }
        std::shared_ptr<SystemNode> node =
            std::make_shared<SystemNode>(schedule, func);
        node->m_in_sets = sets.sets;
        node->m_worker_name = worker_name;
        node->m_system_ptrs_before.insert(
            befores.ptrs.begin(), befores.ptrs.end()
        );
        node->m_system_ptrs_after.insert(
            afters.ptrs.begin(), afters.ptrs.end()
        );
        m_systems.insert({(void*)func, node});
        return node;
    }
};

struct Worker {
    std::string name;
    Worker();
    Worker(std::string str);
};

struct AppExit {};

bool check_exit(EventReader<AppExit> reader);

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
        PluginCache() = default;
        template <typename T>
        PluginCache(T&& t) {
            m_plugin_type = &typeid(T);
        }
        void add_system(void* system_ptr);
    };

    struct AddSystemReturn {
       private:
        std::shared_ptr<SystemNode> m_system;
        App* m_app;

       public:
        AddSystemReturn(App* app, std::weak_ptr<SystemNode> sys);
        App* operator->();
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
        AddSystemReturn& use_worker(std::string name);
        AddSystemReturn& run_if(std::shared_ptr<BasicSystem<bool>> cond);
        AddSystemReturn& get_node(std::shared_ptr<SystemNode>& node);
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

    void update_states();

   public:
    App();

    App* operator->();

    App& log_level(spdlog::level::level_enum level);

    App& enable_loop();

    template <typename T, typename... Args>
    App& configure_sets(T set, Args... sets) {
        m_runner.configure_sets(set, sets...);
        return *this;
    }

    template <typename... Args, typename... TupleT>
    AddSystemReturn add_system(
        Schedule schedule, void (*func)(Args...), TupleT... modifiers
    ) {
        auto mod_tuple = std::make_tuple(modifiers...);
        auto ptr = m_runner.add_system(
            schedule, func, app_tools::tuple_get<Worker>(mod_tuple).name,
            app_tools::tuple_get<after>(mod_tuple),
            app_tools::tuple_get<before>(mod_tuple),
            app_tools::tuple_get<in_set>(mod_tuple)
        );
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
    App& add_plugin(T&& plugin) {
        if (m_plugin_caches.find(&typeid(T)) != m_plugin_caches.end()) {
            logger->warn(
                "Trying to add plugin {}, which already exists.",
                typeid(T).name()
            );
            return *this;
        }
        m_building_plugin_type = &typeid(T);
        PluginCache cache(plugin);
        m_plugin_caches.insert({m_building_plugin_type, cache});
        m_world.insert_resource(plugin);
        plugin.build(*this);
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
                typeid(T).name()
            );
        }
        return *this;
    }

    void run();

    template <typename T>
    App& insert_state(T state) {
        m_world.insert_state(state);
        m_state_update.push_back(
            std::make_unique<
                System<Resource<State<T>>, Resource<NextState<T>>>>(
                state_update<T>()
            )
        );
        return *this;
    }

    template <typename T>
    App& init_state() {
        m_world.init_state<T>();
        m_state_update.push_back(
            std::make_unique<
                System<Resource<State<T>>, Resource<NextState<T>>>>(
                state_update<T>()
            )
        );
        return *this;
    }
};
}  // namespace app
}  // namespace pixel_engine