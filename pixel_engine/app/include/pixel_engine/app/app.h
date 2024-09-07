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

namespace pixel_engine {
namespace app {

enum BasicStage { Start, Loop, Exit };

enum StartStage { PreStartup, Startup, PostStartup };

enum LoopStage { StateTransition, First, PreUpdate, Update, PostUpdate, Last };

enum RenderStage { Prepare, PreRender, Render, PostRender };

enum ExitStage { PreShutdown, Shutdown, PostShutdown };

struct SystemStage {
    size_t m_stage_type_hash;
    size_t m_stage_value;
    template <typename T>
    SystemStage(T t) {
        m_stage_type_hash = typeid(T).hash_code();
        m_stage_value = static_cast<size_t>(t);
    }
};

struct SystemSet {
    size_t m_set_type_hash;
    size_t m_set_value;
    template <typename T>
    SystemSet(T t) {
        m_set_type_hash = typeid(T).hash_code();
        m_set_value = static_cast<size_t>(t);
    }
};

struct Schedule {
    size_t m_stage_type_hash;
    size_t m_stage_value;
    std::shared_ptr<BasicSystem<bool>> m_condition;
};

struct SetMap {
    std::unordered_map<size_t, std::vector<size_t>> m_sets;
    template <typename T, typename... Args>
    void configure_sets(T t, Args... args) {
        std::vector<size_t> set = {
            static_cast<size_t>(t), static_cast<size_t>(args)...};
        m_sets[typeid(T).hash_code()] = set;
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

    template <typename... Args>
    SystemNode(World* world, Schedule schedule, void (*func)(Args...)) {
        m_schedule = schedule;
        m_system = std::make_shared<System<Args...>>(world, func);
        m_func_addr = func;
    }

    double reach_time() {
        if (m_reach_time.has_value()) {
            return m_reach_time.value();
        } else {
            double max_time = 0.0;
            for (auto system : m_systems_before) {
                if (auto system_ptr = system.lock()) {
                    max_time = std::max(max_time, system_ptr->reach_time());
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

struct World {
   private:
    entt::registry m_registry;
    std::unordered_map<entt::entity, std::unordered_set<entt::entity>>
        m_entity_tree;
    std::unordered_map<size_t, std::shared_ptr<void>> m_resources;
    std::unordered_map<size_t, std::shared_ptr<void>> m_events;

    template <typename T, typename... Args>
    T tuple_get(std::tuple<Args...> tuple) {
        using namespace app_tools;
        if constexpr (tuple_contain<T, std::tuple<Args...>>::value) {
            return std::get<T>(tuple);
        } else {
            return T();
        }
    }

    template <template <typename...> typename T, typename... Args>
    constexpr auto tuple_get_template(std::tuple<Args...> tuple) {
        using namespace app_tools;
        if constexpr (tuple_contain_template<T, std::tuple<Args...>>) {
            return std::get<
                tuple_template_index<T, std::tuple<Args...>>::index()>(tuple);
        } else {
            return T();
        }
    }

    template <typename T>
    struct value_type {
        static T get(World* app) { static_assert(1, "value type not valid."); }
    };

    template <>
    struct value_type<Command> {
        static Command get(World* app) {
            return Command(
                &app->m_registry, &app->m_resources, &app->m_entity_tree);
        }
    };

    template <typename... Gets, typename... Withs, typename... Exs>
    struct value_type<Query<Get<Gets...>, With<Withs...>, Without<Exs...>>> {
        static Query<Get<Gets...>, With<Withs...>, Without<Exs...>> get(
            World* app) {
            return Query<Get<Gets...>, With<Withs...>, Without<Exs...>>(
                app->m_registry);
        }
    };

    template <typename Res>
    struct value_type<Resource<Res>> {
        static Resource<Res> get(World* app) {
            if (app->m_resources.find(typeid(Res).hash_code()) !=
                app->m_resources.end()) {
                return Resource<Res>(
                    &app->m_resources[typeid(Res).hash_code()]);
            } else {
                return Resource<Res>();
            }
        }
    };

    template <typename Evt>
    struct value_type<EventWriter<Evt>> {
        static EventWriter<Evt> get(World* app) {
            if (app->m_events.find(typeid(Evt).hash_code()) ==
                app->m_events.end()) {
                app->m_events[typeid(Evt).hash_code()] =
                    std::static_pointer_cast<void>(
                        std::make_shared<std::deque<Event>>());
            }
            return EventWriter<Evt>(app->m_events[typeid(Evt).hash_code()]);
        }
    };

    template <typename Evt>
    struct value_type<EventReader<Evt>> {
        static EventReader<Evt> get(World* app) {
            if (app->m_events.find(typeid(Evt).hash_code()) ==
                app->m_events.end()) {
                app->m_events[typeid(Evt).hash_code()] =
                    std::static_pointer_cast<void>(
                        std::make_shared<std::deque<Event>>());
            }
            return EventReader<Evt>(app->m_events[typeid(Evt).hash_code()]);
        }
    };

    /*!
     * @brief This is where the systems get their parameters by
     * type. All possible type should be handled here.
     */
    template <typename... Args>
    std::tuple<Args...> get_values() {
        return std::make_tuple(value_type<Args>::get(this)...);
    }

    /*! @brief Get a command object on this app.
     * @return The command object.
     */
    Command command() {
        return Command(&m_registry, &m_resources, &m_entity_tree);
    }

   public:
    std::mutex m_system_mutex;

    /*! @brief Insert a state.
     * If the state already exists, nothing will happen.
     * @tparam T The type of the state.
     * @param state The state value to be set when inserted.
     * @return The App object itself.
     */
    template <typename T>
    World& insert_state(T state) {
        Command cmd = command();
        cmd.insert_resource(State(state));
        cmd.insert_resource(NextState(state));

        return *this;
    }

    /*! @brief Insert a state using default values.
     * If the state already exists, nothing will happen.
     * @tparam T The type of the state.
     * @return The App object itself.
     */
    template <typename T>
    World& init_state() {
        Command cmd = command();
        cmd.init_resource<State<T>>();
        cmd.init_resource<NextState<T>>();

        return *this;
    }

    /**
     * @brief Insert a resource.
     *
     * @tparam T The type of the resource.
     * @param res The resource to be inserted.
     * @return World& The App object itself.
     */
    template <typename T>
    World& insert_resource(T res) {
        Command cmd = command();
        cmd.insert_resource(res);
        return *this;
    }

    /**
     * @brief Insert a resource.
     *
     * @tparam T The type of the resource.
     * @tparam Args The types of the arguments to be passed to the constructor
     * of T
     * @param args The arguments to be passed to the constructor of T
     * @return World& The App object itself.
     */
    template <typename T, typename... Args>
    World& insert_resource(Args... args) {
        Command cmd = command();
        cmd.insert_resource<T>(args...);
        return *this;
    }

    /**
     * @brief Insert a resource using default values.
     *
     * @tparam T The type of the resource.=
     * @return World& The App object itself.
     */
    template <typename T>
    World& init_resource() {
        Command cmd = command();
        cmd.init_resource<T>();
        return *this;
    }

    /*! @brief Run a system.
     * @tparam Args The types of the arguments for the system.
     * @param func The system to be run.
     * @return The App object itself.
     */
    template <typename... Args>
    World& run_system(std::function<void(Args...)> func) {
        std::apply(func, get_values<Args...>());
        return *this;
    }

    /*! @brief Run a system.
     * @tparam Args The types of the arguments for the system.
     * @param func The system to be run.
     * @return The App object itself.
     */
    template <typename... Args>
    World& run_system(void (*func)(Args...)) {
        std::apply(func, get_values<Args...>());
        return *this;
    }

    /*! @brief Run a system.
     * @tparam Args1 The types of the arguments for the system.
     * @tparam Args2 The types of the arguments for the condition.
     * @param func The system to be run.
     * @param condition The condition for the system to be run.
     * @return The App object itself.
     */
    template <typename T, typename... Args>
    T run_system_v(std::function<T(Args...)> func) {
        return std::apply(func, get_values<Args...>());
    }

    /*! @brief Run a system.
     * @tparam Args1 The types of the arguments for the system.
     * @tparam Args2 The types of the arguments for the condition.
     * @param func The system to be run.
     * @param condition The condition for the system to be run.
     * @return The App object itself.
     */
    template <typename T, typename... Args>
    T run_system_v(T (*func)(Args...)) {
        return std::apply(func, get_values<Args...>());
    }
};

struct MsgQueue {
    std::queue<std::weak_ptr<SystemNode>> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    void push(std::weak_ptr<SystemNode> system) {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_queue.push(system);
        m_cv.notify_one();
    }
    std::weak_ptr<SystemNode> pop() {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cv.wait(lk, [&] { return !m_queue.empty(); });
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
    size_t m_stage_type_hash;
    size_t m_stage_value;
    std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
        m_workers;
    std::vector<std::weak_ptr<SystemNode>> m_systems;

    std::shared_ptr<SystemNode> m_head;

    MsgQueue m_msg_queue;

   public:
    template <typename StageT>
    StageRunner(
        World* world, StageT stage,
        std::shared_ptr<
            std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>>
            workers)
        : m_world(world),
          m_stage_type_hash(typeid(StageT).hash_code()),
          m_stage_value(static_cast<size_t>(stage)),
          m_workers(workers) {}

    void build(
        const std::unordered_map<void*, std::shared_ptr<SystemNode>>& systems) {
        m_systems.clear();
        for (auto& [addr, system] : systems) {
            if (system->m_schedule.m_stage_type_hash == m_stage_type_hash &&
                system->m_schedule.m_stage_value == m_stage_value) {
                m_systems.push_back(system);
                for (auto before_ptr : system->m_system_ptrs_before) {
                    if (systems.find(before_ptr) != systems.end()) {
                        auto sys_before = systems.find(before_ptr)->second;
                        system->m_systems_before.insert(sys_before);
                        sys_before->m_systems_after.insert(system);
                    }
                }
            }
        }
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
        m_head->clear_temp();
        for (auto system : m_systems) {
            if (auto system_ptr = system.lock()) {
                if (system_ptr->m_system_ptrs_before.empty()) {
                    m_head->m_systems_after.insert(system);
                    system_ptr->m_systems_before.insert(m_head);
                    system_ptr->m_prev_cout = 1;
                }
            }
        }
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
        for (auto system : m_systems) {
            if (auto system_ptr = system.lock()) {
                system_ptr->m_prev_cout = system_ptr->m_temp_before.size() +
                                          system_ptr->m_systems_before.size();
            }
        }
    }

    void wait() {
        for (auto& [name, worker] : *m_workers) {
            worker->wait();
        }
    }

    void notify(std::weak_ptr<SystemNode> system) { m_msg_queue.push(system); }

    void run(std::weak_ptr<SystemNode> system) {
        if (auto system_ptr = system.lock()) {
            auto worker = (*m_workers)[system_ptr->m_worker_name];
            auto ftr = worker->submit_task([=] {
                if (system_ptr->m_system &&
                    (!system_ptr->m_schedule.m_condition ||
                     system_ptr->m_schedule.m_condition->run())) {
                    system_ptr->m_system->run();
                }
                notify(system);
            });
        }
    }

    void run() {
        size_t m_unfinished_system_count;
        run(m_head);
        while (m_unfinished_system_count > 0) {
            m_msg_queue.wait();
            if (!m_msg_queue.empty()) {
                assert(false);
            }
            auto system = m_msg_queue.front();
            m_unfinished_system_count--;
            if (auto system_ptr = system.lock()) {
                for (auto& next : system_ptr->m_systems_after) {
                    if (auto next_ptr = next.lock()) {
                        next_ptr->m_prev_cout--;
                        if (next_ptr->m_prev_cout == 0) {
                            run(next);
                        }
                    }
                }
                for (auto& next : system_ptr->m_temp_after) {
                    if (auto next_ptr = next.lock()) {
                        next_ptr->m_prev_cout--;
                        if (next_ptr->m_prev_cout == 0) {
                            run(next);
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
    void add_worker(std::string name, size_t num_threads) {
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
            StageRunner(m_world, stages, &m_workers, &m_systems)...};
        m_stages[stage] = stage_runners;
    }
    void run_stage(BasicStage stage) {
        for (auto& stage_runner : m_stages[stage]) {
            stage_runner.prepare();
            stage_runner.run();
            stage_runner.wait();
        }
    }
    void build() {
        for (auto& [ptr, system] : m_systems) {
            system->m_systems_after.clear();
            system->m_systems_before.clear();
        }
        for (auto& stage_runner : m_stages[BasicStage::Start]) {
            stage_runner.build(m_systems);
        }
    }

    template <typename... Args>
    std::weak_ptr<SystemNode> add_system(
        Schedule schedule, void (*func)(Args...),
        std::string worker_name = "default", after befores = after(),
        before afters = before(), in_set sets = in_set()) {
        if (m_systems.find(func) != m_systems.end()) {
            spdlog::error(
                "System {} : {} already exists.", (void*)func,
                typeid(func).name());
            throw std::runtime_error("System already exists.");
        }
        std::shared_ptr<SystemNode> node =
            std::make_shared<SystemNode>(m_world, schedule, func);
        node->m_in_sets = sets.sets;
        node->m_system_ptrs_before = befores.ptrs;
        node->m_system_ptrs_after = afters.ptrs;
        m_systems.insert({(void*)func, node});
        return node;
    }
};

struct Worker {
    std::string name;
    Worker(std::string str) : name(str) {}
};

struct App {
   private:
    World m_world;
    Runner m_runner = Runner({{"default", 8}, {"single", 1}});
    bool m_loop_enabled = false;

    struct AddSystemReturn {
       private:
        std::weak_ptr<SystemNode> m_system;
        App* m_app;

       public:
        AddSystemReturn(App* app, std::weak_ptr<SystemNode> sys)
            : m_app(app), m_system(sys) {}
        App* operator->() { return m_app; }
        template <typename... Sets>
        AddSystemReturn& in_set(Sets... sets) {
            if (auto sys_ptr = m_system.lock()) {
                app::in_set sets_container = app::in_set(sets...);
                for (auto& sys_set : sets_container.sets) {
                    sys_ptr->m_in_sets.push_back(sys_set);
                }
            }
            return *this;
        }
        template <typename... Addrs>
        AddSystemReturn& before(Addrs... addrs) {
            if (auto sys_ptr = m_system.lock()) {
                std::vector<void*> addrs_v = {((void*)addrs)...};
                for (auto ptr : addrs_v) {
                    sys_ptr->m_system_ptrs_after.insert(ptr);
                }
            }
        }
        template <typename... Addrs>
        AddSystemReturn& after(Addrs... addrs) {
            if (auto sys_ptr = m_system.lock()) {
                std::vector<void*> addrs_v = {((void*)addrs)...};
                for (auto ptr : addrs_v) {
                    sys_ptr->m_system_ptrs_before.insert(ptr);
                }
            }
            return *this;
        }
        AddSystemReturn& use_worker(std::string name) {
            if (auto sys_ptr = m_system.lock()) {
                sys_ptr->m_worker_name = name;
            }
            return *this;
        }
    };

   public:
    App() {
        m_runner.m_world = &m_world;
        m_runner.configure_stage(
            BasicStage::Start, StartStage::PreStartup, StartStage::Startup,
            StartStage::PostStartup);
        m_runner.configure_stage(
            BasicStage::Loop, LoopStage::First, LoopStage::StateTransition,
            LoopStage::PreUpdate, LoopStage::Update, LoopStage::PostUpdate,
            LoopStage::Last, RenderStage::Prepare, RenderStage::PreRender,
            RenderStage::Render, RenderStage::PostRender);
    }

    App* operator->() { return this; }

    App& enable_loop() { m_loop_enabled = true; }

    template <typename... Args, typename... TupleT>
    AddSystemReturn add_system(
        Schedule schedule, void (*func)(Args...), TupleT... modifiers) {
        auto mod_tuple = std::make_tuple(modifiers...);
        auto ptr = m_runner.add_system(
            schedule, func, app_tools::tuple_get<Worker>(mod_tuple).name,
            app_tools::tuple_get<after>(mod_tuple),
            app_tools::tuple_get<before>(mod_tuple),
            app_tools::tuple_get<in_set>(mod_tuple));
        return AddSystemReturn(this, ptr);
    }

    void run() {
        m_runner.build();
        m_runner.run_stage(BasicStage::Start);
        do {
            m_runner.run_stage(BasicStage::Loop);
        } while (m_loop_enabled);
        m_runner.run_stage(BasicStage::Exit);
    }
};
}  // namespace app
}  // namespace pixel_engine