#pragma once

#include <BS_thread_pool.hpp>
#include <entt/entity/registry.hpp>
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
};

struct StageRunner {
   private:
    App* m_app;
    size_t m_stage_type_hash;
    size_t m_stage_value;
    std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
        m_workers;
    std::vector<std::weak_ptr<SystemNode>> m_systems;
    std::unordered_map<std::weak_ptr<SystemNode>, std::future<void>> m_futures;
    std::mutex* m_system_mutex;

   public:
    template <typename StageT>
    StageRunner(
        App* app, StageT stage,
        std::shared_ptr<
            std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>>
            workers,
        std::unordered_map<void*, std::shared_ptr<SystemNode>> systems)
        : m_app(app),
          m_stage_type_hash(typeid(StageT).hash_code()),
          m_stage_value(static_cast<size_t>(stage)),
          m_workers(workers) {
        m_app = app;
        for (auto& [addr, system] : systems) {
            if (system->m_schedule.m_stage_type_hash == m_stage_type_hash &&
                system->m_schedule.m_stage_value == m_stage_value) {
                m_systems.push_back(system);
            }
        }
    }

    void prepare() {
        m_system_mutex->lock();
        std::erase_if(m_systems, [](auto system) { return system.expired(); });
        for (auto system : m_systems) {
            if (auto system_ptr = system.lock()) {
                system_ptr->reset_reach_time();
            }
        }
        std::sort(m_systems.begin(), m_systems.end(), [](auto a, auto b) {
            auto a_ptr = a.lock();
            auto b_ptr = b.lock();
            return a_ptr->reach_time() < b_ptr->reach_time();
        });
        m_system_mutex->unlock();
        m_futures.clear();
    }

    void wait() {
        for (auto& [name, worker] : *m_workers) {
            worker->wait();
        }
    }

    std::mutex m_mutex;

    void run() {
        std::unordered_set<std::weak_ptr<SystemNode>> systems_all(
            m_systems.begin(), m_systems.end());
        std::deque<std::weak_ptr<SystemNode>> to_run;
        std::condition_variable cv;
        for (auto system : m_systems) {
            if (auto system_ptr = system.lock()) {
                if (system_ptr->m_systems_before.size() ||
                    system_ptr->m_temp_before.size()) {
                    continue;
                }
                if (m_futures.find(system) != m_futures.end()) continue;
                auto worker = m_workers->at(system_ptr->m_worker_name);
                m_futures[system] = worker->submit_task([&]() {
                    bool run = !system_ptr->m_schedule.m_condition ||
                               system_ptr->m_schedule.m_condition->run();
                    system_ptr->m_system->run();
                    std::unique_lock<std::mutex> lk(m_mutex);
                    for (auto& after : system_ptr->m_systems_after) {
                        if (auto after_ptr = after.lock()) {
                            to_run.push_back(after);
                        }
                    }
                    for (auto& after : system_ptr->m_temp_after) {
                        if (auto after_ptr = after.lock()) {
                            to_run.push_back(after);
                        }
                    }
                    cv.notify_all();
                });
                systems_all.erase(system);
            }
        }
        while (systems_all.size()) {
            for (auto system : to_run) {
                if (auto system_ptr = system.lock()) {
                    if (m_futures.find(system) != m_futures.end()) continue;
                    bool can_run = true;
                    std::unique_lock<std::mutex> lk(m_mutex);
                    for (auto& before : system_ptr->m_systems_before) {
                        if (auto before_ptr = before.lock()) {
                            if (m_futures.find(before) == m_futures.end()) {
                                can_run = false;
                                break;
                            }
                            auto& future = m_futures[before];
                            if (future.wait_for(std::chrono::seconds(0)) !=
                                std::future_status::ready) {
                                can_run = false;
                                break;
                            }
                        }
                    }
                    for (auto& before : system_ptr->m_temp_before) {
                        if (auto before_ptr = before.lock()) {
                            if (m_futures.find(before) == m_futures.end()) {
                                can_run = false;
                                break;
                            }
                            auto& future = m_futures[before];
                            if (future.wait_for(std::chrono::seconds(0)) !=
                                std::future_status::ready) {
                                can_run = false;
                                break;
                            }
                        }
                    }
                    lk.unlock();
                    if (!can_run) continue;
                    auto worker = m_workers->at(system_ptr->m_worker_name);
                    m_futures[system] = worker->submit_task([&]() {
                        bool run = !system_ptr->m_schedule.m_condition ||
                                   system_ptr->m_schedule.m_condition->run();
                        system_ptr->m_system->run();
                        std::unique_lock<std::mutex> lk(m_mutex);
                        for (auto& after : system_ptr->m_systems_after) {
                            if (auto after_ptr = after.lock()) {
                                to_run.push_back(after);
                            }
                        }
                        for (auto& after : system_ptr->m_temp_after) {
                            if (auto after_ptr = after.lock()) {
                                to_run.push_back(after);
                            }
                        }
                        cv.notify_all();
                    });
                    systems_all.erase(system);
                }
            }
            std::unique_lock<std::mutex> lk(m_mutex);
            cv.wait(lk);
        }
    }
};

struct Runner {
    std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>> m_workers;
    std::unordered_map<BasicStage, std::vector<StageRunner>> m_stages;
    std::unordered_map<void*, std::shared_ptr<SystemNode>>* m_systems;
    void add_worker(std::string name, size_t num_threads) {
        m_workers.insert(
            {name, std::make_shared<BS::thread_pool>(num_threads)});
    }
    Runner(
        std::vector<std::pair<std::string, size_t>> workers,
        std::unordered_map<void*, std::shared_ptr<SystemNode>>* systems)
        : m_systems(systems) {
        for (auto worker : workers) {
            add_worker(worker.first, worker.second);
        }
    }
    template <typename... SubStages>
    void configure_stage(App* app, BasicStage stage, SubStages... stages) {
        std::vector<StageRunner> stage_runners = {
            StageRunner(app, stages, &m_workers, m_systems)...};
        m_stages[stage] = stage_runners;
    }
    void run_stage(BasicStage stage) {
        for (auto& stage_runner : m_stages[stage]) {
            stage_runner.prepare();

            stage_runner.wait();
        }
    }
};

struct App {
   private:
    bool m_loop = false;
    entt::registry m_registry;
    std::unordered_map<entt::entity, std::unordered_set<entt::entity>>
        m_entity_tree;
    std::unordered_map<size_t, std::shared_ptr<void>> m_resources;
    std::unordered_map<size_t, std::shared_ptr<void>> m_events;
    std::unordered_map<void*, std::shared_ptr<SystemNode>> m_systems;
    Runner m_runner = Runner({{"default", 4}, {"single", 1}}, &m_systems);

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
        static T get(App* app) { static_assert(1, "value type not valid."); }
    };

    template <>
    struct value_type<Command> {
        static Command get(App* app) {
            return Command(
                &app->m_registry, &app->m_resources, &app->m_entity_tree);
        }
    };

    template <typename... Gets, typename... Withs, typename... Exs>
    struct value_type<Query<Get<Gets...>, With<Withs...>, Without<Exs...>>> {
        static Query<Get<Gets...>, With<Withs...>, Without<Exs...>> get(
            App* app) {
            return Query<Get<Gets...>, With<Withs...>, Without<Exs...>>(
                app->m_registry);
        }
    };

    template <typename Res>
    struct value_type<Resource<Res>> {
        static Resource<Res> get(App* app) {
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
        static EventWriter<Evt> get(App* app) {
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
        static EventReader<Evt> get(App* app) {
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
    App& insert_state(T state) {
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
    App& init_state() {
        Command cmd = command();
        cmd.init_resource<State<T>>();
        cmd.init_resource<NextState<T>>();

        return *this;
    }

    /*! @brief Run a system.
     * @tparam Args The types of the arguments for the system.
     * @param func The system to be run.
     * @return The App object itself.
     */
    template <typename... Args>
    App& run_system(std::function<void(Args...)> func) {
        std::apply(func, get_values<Args...>());
        return *this;
    }

    /*! @brief Run a system.
     * @tparam Args The types of the arguments for the system.
     * @param func The system to be run.
     * @return The App object itself.
     */
    template <typename... Args>
    App& run_system(void (*func)(Args...)) {
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
}  // namespace app
}  // namespace pixel_engine