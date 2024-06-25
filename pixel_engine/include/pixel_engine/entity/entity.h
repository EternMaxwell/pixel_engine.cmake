#pragma once

#include <concurrent_unordered_set.h>
#include <spdlog/spdlog.h>

#include <BS_thread_pool.hpp>
#include <any>
#include <deque>
#include <entt/entity/registry.hpp>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <pfr.hpp>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "pixel_engine/entity/command.h"
#include "pixel_engine/entity/event.h"
#include "pixel_engine/entity/query.h"
#include "pixel_engine/entity/resource.h"
#include "pixel_engine/entity/scheduler.h"
#include "pixel_engine/entity/state.h"
#include "pixel_engine/entity/system.h"

namespace pixel_engine {
    namespace entity {
        class Plugin {
           public:
            virtual void build(App& app) = 0;
        };

        class LoopPlugin;

        struct AppExit {};

        static bool check_exit(EventReader<AppExit> exit_events) {
            spdlog::debug("Check if exit");
            for (auto& e : exit_events.read()) {
                spdlog::info("Exit event received, exiting app.");
                return true;
            }
            return false;
        }

        void exit_app(EventWriter<AppExit> exit_events) { exit_events.write(AppExit{}); }

        struct SystemNode {
            const bool in_main_thread;
            std::shared_ptr<Scheduler> scheduler;
            std::shared_ptr<BasicSystem<void>> system;
            std::unordered_set<std::shared_ptr<condition>> conditions;
            std::unordered_set<std::shared_ptr<SystemNode>> user_defined_before;
            std::unordered_set<std::shared_ptr<SystemNode>> app_generated_before;
            std::unordered_map<size_t, std::any> sets;
            double avg_reach_time = 0;

            SystemNode(
                std::shared_ptr<Scheduler> scheduler, std::shared_ptr<BasicSystem<void>> system,
                const bool& in_main = false)
                : scheduler(scheduler), system(system), in_main_thread(in_main) {}

            std::tuple<std::shared_ptr<Scheduler>, std::shared_ptr<BasicSystem<void>>> to_tuple() {
                return std::make_tuple(scheduler, system);
            }
            /*! @brief Get the depth of a node, 0 if it is a leaf node.
             * @return The depth of the node.
             */
            size_t user_before_depth() {
                size_t max_depth = 0;
                for (auto& before : user_defined_before) {
                    max_depth = std::max(max_depth, before->user_before_depth() + 1);
                }
                return max_depth;
            }
            /*! @brief Get the time in milliseconds for the runner to reach this system.
             * @return The time in milliseconds.
             */
            double time_to_reach() {
                double max_time = 0;
                for (auto& before : user_defined_before) {
                    max_time = std::max(max_time, before->time_to_reach() + before->system->get_avg_time());
                }
                return max_time;
            }
        };

        typedef std::shared_ptr<SystemNode> Node;

        struct SystemRunner {
           public:
            SystemRunner(App* app, std::shared_ptr<BS::thread_pool> pool)
                : app(app), ignore_scheduler(false), pool(pool) {}
            SystemRunner(App* app, std::shared_ptr<BS::thread_pool> pool, bool ignore_scheduler)
                : app(app), ignore_scheduler(ignore_scheduler), pool(pool) {}
            SystemRunner(const SystemRunner& other) {
                app = other.app;
                ignore_scheduler = other.ignore_scheduler;
                tails = other.tails;
                pool = other.pool;
            }
            void run();
            void add_system(std::shared_ptr<SystemNode> node) { systems_all.push_back(node); }
            void prepare();
            void reset() { futures.clear(); }
            void wait() { pool->wait(); }
            void sort_time() {
                std::sort(systems_all.begin(), systems_all.end(), [](const auto& a, const auto& b) {
                    return a->time_to_reach() < b->time_to_reach();
                });
            }
            void sort_depth() {
                std::sort(systems_all.begin(), systems_all.end(), [](const auto& a, const auto& b) {
                    return a->user_before_depth() < b->user_before_depth();
                });
            }
            size_t system_count() { return systems_all.size(); }

           private:
            App* app;
            bool ignore_scheduler;
            bool runned = false;
            std::condition_variable cv;
            std::mutex m;
            std::atomic<bool> any_done = true;
            std::unordered_set<std::shared_ptr<SystemNode>> tails;
            std::vector<std::shared_ptr<SystemNode>> systems_all;
            std::unordered_map<std::shared_ptr<SystemNode>, std::future<void>> futures;
            std::shared_ptr<BS::thread_pool> pool;
            std::shared_ptr<SystemNode> should_run(const std::shared_ptr<SystemNode>& sys);
            std::shared_ptr<SystemNode> get_next();
            bool done();
            bool all_load();
        };

        void SystemRunner::run() {
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

        void SystemRunner::prepare() {
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

        std::shared_ptr<SystemNode> SystemRunner::should_run(const std::shared_ptr<SystemNode>& sys) {
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

        std::shared_ptr<SystemNode> SystemRunner::get_next() {
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

        bool SystemRunner::done() {
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

        bool SystemRunner::all_load() {
            for (auto& sys : tails) {
                if (futures.find(sys) == futures.end()) {
                    return false;
                }
            }
            return true;
        }

        template <typename T, typename Tuple>
        struct tuple_contain;

        template <typename T>
        struct tuple_contain<T, std::tuple<>> : std::false_type {};

        template <typename T, typename U, typename... Ts>
        struct tuple_contain<T, std::tuple<U, Ts...>> : tuple_contain<T, std::tuple<Ts...>> {};

        template <typename T, typename... Ts>
        struct tuple_contain<T, std::tuple<T, Ts...>> : std::true_type {};

        template <template <typename...> typename T, typename Tuple>
        struct tuple_contain_template;

        template <template <typename...> typename T>
        struct tuple_contain_template<T, std::tuple<>> : std::false_type {};

        template <template <typename...> typename T, typename U, typename... Ts>
        struct tuple_contain_template<T, std::tuple<U, Ts...>> : tuple_contain_template<T, std::tuple<Ts...>> {};

        template <template <typename...> typename T, typename... Ts, typename... Temps>
        struct tuple_contain_template<T, std::tuple<T<Temps...>, Ts...>> : std::true_type {};

        template <template <typename...> typename T, typename Tuple>
        struct tuple_template_index {};

        template <template <typename...> typename T, typename U, typename... Args>
        struct tuple_template_index<T, std::tuple<U, Args...>> {
            static constexpr int index() {
                if constexpr (is_template_of<T, U>::value) {
                    return 0;
                } else {
                    return 1 + tuple_template_index<T, std::tuple<Args...>>::index();
                }
            }
        };

        struct before {
            friend class App;

           private:
            std::vector<std::shared_ptr<SystemNode>> nodes;

           public:
            before(){};
            template <typename... Nodes>
            before(Nodes... nodes) : nodes({nodes...}){};
        };

        struct after {
            friend class App;

           private:
            std::vector<std::shared_ptr<SystemNode>> nodes;

           public:
            after(){};
            template <typename... Nodes>
            after(Nodes... nodes) : nodes({nodes...}){};
        };

        template <typename... Args>
        struct in_set {
            std::unordered_map<size_t, std::any> sets;
            in_set(Args... args) : sets({{typeid(Args).hash_code(), std::any(args)}...}) {}
            template <typename T, typename... Args>
            in_set(in_set<T, Args...> in_sets) : sets(in_sets.sets) {}
            in_set(std::unordered_map<size_t, std::any> sets) : sets(sets) {}
        };

        class App {
            friend class LoopPlugin;

           protected:
            bool m_loop_enabled = false;
            entt::registry m_registry;
            std::unordered_map<entt::entity, std::set<entt::entity>> m_entity_relation_tree;
            std::unordered_map<size_t, std::any> m_resources;
            std::unordered_map<size_t, std::deque<Event>> m_events;
            std::unordered_map<size_t, std::vector<std::any>> m_system_sets;
            std::unordered_map<size_t, std::vector<std::shared_ptr<SystemNode>>> m_in_set_systems;
            std::vector<Command> m_existing_commands;
            std::vector<std::unique_ptr<BasicSystem<void>>> m_state_update;
            std::vector<std::shared_ptr<SystemNode>> m_systems;
            std::unordered_map<size_t, std::shared_ptr<Plugin>> m_plugins;
            std::unordered_map<size_t, std::shared_ptr<SystemRunner>> m_runners;
            std::shared_ptr<BS::thread_pool> m_pool = std::make_shared<BS::thread_pool>(4);

            void enable_loop() { m_loop_enabled = true; }
            void disable_loop() { m_loop_enabled = false; }

            template <typename T, typename... Args>
            T tuple_get(std::tuple<Args...> tuple) {
                if constexpr (tuple_contain<T, std::tuple<Args...>>::value) {
                    return std::get<T>(tuple);
                } else {
                    if constexpr (std::is_same_v<T, std::shared_ptr<SystemNode>*>) {
                        return nullptr;
                    }
                    return T();
                }
            }

            template <template <typename...> typename T, typename... Args>
            constexpr auto tuple_get_template(std::tuple<Args...> tuple) {
                if constexpr (tuple_contain_template<T, std::tuple<Args...>>::value) {
                    return std::get<tuple_template_index<T, std::tuple<Args...>>::index()>(tuple);
                } else {
                    return T();
                }
            }

            template <typename T>
            struct get_resource {};

            template <template <typename...> typename Template, typename Arg>
            struct get_resource<Template<Arg>> {
                friend class App;
                friend class Resource<Arg>;

               private:
                App& m_app;

               public:
                get_resource(App& app) : m_app(app) {}

                Resource<Arg> value() {
                    if (m_app.m_resources.find(typeid(Arg).hash_code()) != m_app.m_resources.end()) {
                        return Resource<Arg>(&m_app.m_resources[typeid(Arg).hash_code()]);
                    } else {
                        return Resource<Arg>();
                    }
                }
            };

            template <typename T>
            struct get_event_writer {};

            template <template <typename...> typename Template, typename Arg>
            struct get_event_writer<Template<Arg>> {
                friend class App;
                friend class EventWriter<Arg>;

               private:
                App& m_app;

               public:
                get_event_writer(App& app) : m_app(app) {}

                EventWriter<Arg> value() { return EventWriter<Arg>(&m_app.m_events[typeid(Arg).hash_code()]); }
            };

            template <typename T>
            struct get_event_reader {};

            template <template <typename...> typename Template, typename Arg>
            struct get_event_reader<Template<Arg>> {
               private:
                App& m_app;

               public:
                get_event_reader(App& app) : m_app(app) {}

                EventReader<Arg> value() { return EventReader<Arg>(&m_app.m_events[typeid(Arg).hash_code()]); }
            };

            /*!
             * @brief This is where the systems get their parameters by
             * type. All possible type should be handled here.
             */
            template <typename T, typename... Args>
            std::tuple<T, Args...> get_values_internal() {
                static_assert(
                    std::same_as<T, Command> || is_template_of<query::Query, T>::value ||
                        is_template_of<Resource, T>::value || is_template_of<EventReader, T>::value ||
                        is_template_of<EventWriter, T>::value,
                    "an illegal argument type is used in systems.");
                if constexpr (std::same_as<T, Command>) {
                    m_existing_commands.push_back(command());
                    if constexpr (sizeof...(Args) > 0) {
                        return std::tuple_cat(std::make_tuple(m_existing_commands.back()), get_values<Args...>());
                    } else {
                        return std::make_tuple(m_existing_commands.back());
                    }
                } else if constexpr (is_template_of<query::Query, T>::value) {
                    T query(m_registry);
                    if constexpr (sizeof...(Args) > 0) {
                        return std::tuple_cat(std::make_tuple(query), get_values<Args...>());
                    } else {
                        return std::make_tuple(query);
                    }
                } else if constexpr (is_template_of<Resource, T>::value) {
                    T res = get_resource<T>(*this).value();
                    if constexpr (sizeof...(Args) > 0) {
                        return std::tuple_cat(std::make_tuple(res), get_values<Args...>());
                    } else {
                        return std::make_tuple(res);
                    }
                } else if constexpr (is_template_of<EventReader, T>::value) {
                    T reader = get_event_reader<T>(*this).value();
                    if constexpr (sizeof...(Args) > 0) {
                        return std::tuple_cat(std::make_tuple(reader), get_values<Args...>());
                    } else {
                        return std::make_tuple(reader);
                    }
                } else if constexpr (is_template_of<EventWriter, T>::value) {
                    T writer = get_event_writer<T>(*this).value();
                    if constexpr (sizeof...(Args) > 0) {
                        return std::tuple_cat(std::make_tuple(writer), get_values<Args...>());
                    } else {
                        return std::make_tuple(writer);
                    }
                }
            }

            template <typename... Args>
            std::tuple<Args...> get_values() {
                if constexpr (sizeof...(Args) > 0) {
                    return get_values_internal<Args...>();
                } else {
                    return std::make_tuple();
                }
            }

            template <typename T>
            auto state_update() {
                return [&](Resource<State<T>> state, Resource<NextState<T>> state_next) {
                    if (state.has_value() && state_next.has_value()) {
                        state->just_created = false;
                        state->m_state = state_next->m_state;
                    }
                };
            }

            void end_commands() {
                for (auto& cmd : m_existing_commands) {
                    cmd.end();
                }
                m_existing_commands.clear();
            }

            template <typename T>
            void run_systems_scheduled() {
                for (auto& node : m_systems) {
                    auto& scheduler = node->scheduler;
                    auto& system = node->system;
                    if (scheduler != nullptr && dynamic_cast<T*>(scheduler.get()) != NULL &&
                        scheduler->should_run(this)) {
                        system->run();
                    }
                }
            }

            void run_state_change_systems() { run_systems_scheduled<OnStateChange>(); }

            void run_start_up_systems() { run_systems_scheduled<Startup>(); }

            void run_update_systems() { run_systems_scheduled<Update>(); }

            void run_render_systems() { run_systems_scheduled<Render>(); }

            void tick_events() {
                for (auto& [key, value] : m_events) {
                    while (!value.empty()) {
                        auto& event = value.front();
                        if (event.ticks >= 1) {
                            value.pop_front();
                        } else {
                            break;
                        }
                    }
                    for (auto& event : value) {
                        event.ticks++;
                    }
                }
            }

            void update_states() {
                for (auto& state : m_state_update) {
                    state->run();
                }
            }

            template <typename SchT, typename T, typename... Args>
            void configure_system_sets(std::shared_ptr<SystemNode> node, in_set<T, Args...> in_sets) {
                T& set_of_T = std::any_cast<T&>(in_sets.sets[typeid(T).hash_code()]);
                if (m_system_sets.find(typeid(T).hash_code()) != m_system_sets.end()) {
                    bool before = true;
                    for (auto& set_any : m_system_sets[typeid(T).hash_code()]) {
                        T& set = std::any_cast<T&>(set_any);
                        if (set == set_of_T) {
                            before = false;
                            break;
                        }
                        for (auto& systems : m_in_set_systems[typeid(T).hash_code()]) {
                            if (systems->sets.find(typeid(T).hash_code()) != systems->sets.end()) {
                                T& sys_set = std::any_cast<T&>(systems->sets[typeid(T).hash_code()]);
                                if (sys_set == set && dynamic_cast<SchT*>(systems->scheduler.get()) != NULL) {
                                    if (before) {
                                        node->user_defined_before.insert(systems);
                                    } else {
                                        systems->user_defined_before.insert(node);
                                    }
                                }
                            }
                        }
                    }
                }
                m_in_set_systems[typeid(T).hash_code()].push_back(node);
                node->sets[typeid(T).hash_code()] = set_of_T;
                if constexpr (sizeof...(Args) > 0) {
                    configure_system_sets<SchT>(node, in_set<Args...>(in_sets.sets));
                }
            }

            template <typename T>
            void configure_system_sets(std::shared_ptr<SystemNode> node, in_set<> in_sets) {}

           public:
            App() {}

            /*! @brief Get the registry.
             * @return The registry.
             */
            const auto& registry() { return m_registry; }

            /*! @brief Get a command object on this app.
             * @return The command object.
             */
            Command command() { return Command(&m_registry, &m_entity_relation_tree, &m_resources, &m_events); }

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

            void check_locked(std::shared_ptr<SystemNode> node, std::shared_ptr<SystemNode>& node2) {
                for (auto& before : node->user_defined_before) {
                    if (before == node2) {
                        throw std::runtime_error("System locked.");
                    }
                    check_locked(before, node2);
                }
            }

            /*! @brief Configure system sets. This means the sequence of the args is the sequence of the sets. And this
             * affects how systems are run.
             *  @tparam T The type of the system set.
             *  @param arg The system set.
             *  @param args The rest of the system sets.
             *  @return The App object itself.
             */
            template <typename T, typename... Args>
            App& configure_sets(T arg, Args... args) {
                m_system_sets[typeid(T).hash_code()] = {arg, args...};
                std::vector<std::vector<std::shared_ptr<SystemNode>>> in_set_systems;
                for (auto& set_any : m_system_sets[typeid(T).hash_code()]) {
                    T& set = std::any_cast<T&>(set_any);
                    in_set_systems.push_back({});
                    auto& systems_in_this_set = in_set_systems.back();
                    for (auto& systems : m_in_set_systems[typeid(T).hash_code()]) {
                        if (systems->sets.find(typeid(T).hash_code()) != systems->sets.end()) {
                            T& sys_set = std::any_cast<T&>(systems->sets[typeid(T).hash_code()]);
                            if (sys_set == set) {
                                systems_in_this_set.push_back(systems);
                            }
                        }
                    }
                }
                for (int i = 0; i < in_set_systems.size(); i++) {
                    for (int j = i + 1; j < in_set_systems.size(); j++) {
                        for (auto& system1 : in_set_systems[i]) {
                            for (auto& system2 : in_set_systems[j]) {
                                system2->user_defined_before.insert(system1);
                            }
                        }
                    }
                }
                return *this;
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @param befores The systems that should run after this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @param afters The systems that should run before this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args, typename... Sets>
            App& add_system_inner(
                Sch scheduler, std::function<void(Args...)> func, std::shared_ptr<SystemNode>* node = nullptr,
                before befores = before(), after afters = after(),
                std::unordered_set<std::shared_ptr<condition>> conditions = {}, in_set<Sets...> in_sets = in_set()) {
                std::shared_ptr<SystemNode> new_node = std::make_shared<SystemNode>(
                    std::make_shared<Sch>(scheduler), std::make_shared<System<Args...>>(System<Args...>(this, func)));
                new_node->conditions = conditions;
                for (auto& before_node : afters.nodes) {
                    if ((before_node != nullptr) && (dynamic_cast<Sch*>(before_node->scheduler.get()) != NULL)) {
                        new_node->user_defined_before.insert(before_node);
                    }
                }
                for (auto& after_node : befores.nodes) {
                    if ((after_node != nullptr) && (dynamic_cast<Sch*>(after_node->scheduler.get()) != NULL)) {
                        after_node->user_defined_before.insert(new_node);
                    }
                }
                if (node != nullptr) {
                    *node = new_node;
                }
                configure_system_sets<Sch>(new_node, in_sets);
                check_locked(new_node, new_node);
                m_systems.push_back(new_node);
                return *this;
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @param befores The systems that should run after this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @param afters The systems that should run before this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args, typename... Ts>
            App& add_system(Sch scheduler, void (*func)(Args...), Ts... args) {
                auto args_tuple = std::tuple<Ts...>(args...);
                auto tuple = std::make_tuple(
                    scheduler, std::function<void(Args...)>(func), tuple_get<std::shared_ptr<SystemNode>*>(args_tuple),
                    tuple_get<before>(args_tuple), tuple_get<after>(args_tuple),
                    tuple_get<std::unordered_set<std::shared_ptr<condition>>>(args_tuple),
                    tuple_get_template<in_set>(args_tuple));
                std::apply([this](auto... args) { add_system_inner(args...); }, tuple);
                return *this;
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @param befores The systems that should run after this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @param afters The systems that should run before this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args, typename... Ts>
            App& add_system(Sch scheduler, std::function<void(Args...)> func, Ts... args) {
                auto args_tuple = std::tuple<Ts...>(args...);
                auto tuple = std::make_tuple(
                    scheduler, func, tuple_get<std::shared_ptr<SystemNode>*>(args_tuple), tuple_get<before>(args_tuple),
                    tuple_get<after>(args_tuple), tuple_get<std::unordered_set<std::shared_ptr<condition>>>(args_tuple),
                    tuple_get_template<in_set>(args_tuple));
                std::apply([this](auto... args) { add_system_inner(args...); }, tuple);
                return *this;
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @param befores The systems that should run after this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @param afters The systems that should run before this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args, typename... Sets>
            App& add_system_main_inner(
                Sch scheduler, std::function<void(Args...)> func, std::shared_ptr<SystemNode>* node = nullptr,
                before befores = before(), after afters = after(),
                std::unordered_set<std::shared_ptr<condition>> conditions = {}, in_set<Sets...> in_sets = in_set()) {
                std::shared_ptr<SystemNode> new_node = std::make_shared<SystemNode>(
                    std::make_shared<Sch>(scheduler), std::make_shared<System<Args...>>(System<Args...>(this, func)),
                    true);
                new_node->conditions = conditions;
                for (auto& before_node : afters.nodes) {
                    if ((before_node != nullptr) && (dynamic_cast<Sch*>(before_node->scheduler.get()) != NULL)) {
                        new_node->user_defined_before.insert(before_node);
                    }
                }
                for (auto& after_node : befores.nodes) {
                    if ((after_node != nullptr) && (dynamic_cast<Sch*>(after_node->scheduler.get()) != NULL)) {
                        after_node->user_defined_before.insert(new_node);
                    }
                }
                if (node != nullptr) {
                    *node = new_node;
                }
                configure_system_sets<Sch>(new_node, in_sets);
                check_locked(new_node, new_node);
                m_systems.push_back(new_node);
                return *this;
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @param befores The systems that should run after this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @param afters The systems that should run before this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args, typename... Ts>
            App& add_system_main(Sch scheduler, void (*func)(Args...), Ts... args) {
                auto args_tuple = std::tuple<Ts...>(args...);
                auto tuple = std::make_tuple(
                    scheduler, std::function<void(Args...)>(func), tuple_get<std::shared_ptr<SystemNode>*>(args_tuple),
                    tuple_get<before>(args_tuple), tuple_get<after>(args_tuple),
                    tuple_get<std::unordered_set<std::shared_ptr<condition>>>(args_tuple),
                    tuple_get_template<in_set>(args_tuple));
                std::apply([this](auto... args) { add_system_main_inner(args...); }, tuple);
                return *this;
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @param befores The systems that should run after this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @param afters The systems that should run before this system. If they are not in same
             * scheduler, this will be ignored when preparing runners.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args, typename... Ts>
            App& add_system_main(Sch scheduler, std::function<void(Args...)> func, Ts... args) {
                auto args_tuple = std::tuple<Ts...>(args...);
                auto tuple = std::make_tuple(
                    scheduler, func, tuple_get<std::shared_ptr<SystemNode>*>(args_tuple), tuple_get<before>(args_tuple),
                    tuple_get<after>(args_tuple), tuple_get<std::unordered_set<std::shared_ptr<condition>>>(args_tuple),
                    tuple_get_template<in_set>(args_tuple));
                std::apply([this](auto... args) { add_system_main_inner(args...); }, tuple);
                return *this;
            }

            /*! @brief Add a plugin.
             * @param plugin The plugin to be added.
             * @return The App object itself.
             */
            template <typename T, std::enable_if_t<std::is_base_of_v<Plugin, T>>* = nullptr>
            App& add_plugin(T plugin) {
                plugin.build(*this);
                m_plugins[typeid(T).hash_code()] = std::make_shared<T>(plugin);
                return *this;
            }

            /*! @brief Get a plugin.
             * @tparam T The type of the plugin.
             * @return The plugin.
             */
            template <typename T>
            std::shared_ptr<T> get_plugin() {
                if (m_plugins.find(typeid(T).hash_code()) != m_plugins.end()) {
                    return std::static_pointer_cast<T>(m_plugins[typeid(T).hash_code()]);
                } else {
                    return nullptr;
                }
            }

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
                m_state_update.push_back(std::make_unique<System<Resource<State<T>>, Resource<NextState<T>>>>(
                    System<Resource<State<T>>, Resource<NextState<T>>>(this, state_update<T>())));
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
                m_state_update.push_back(std::make_unique<System<Resource<State<T>>, Resource<NextState<T>>>>(
                    System<Resource<State<T>>, Resource<NextState<T>>>(this, state_update<T>())));
                return *this;
            }

            /*! @brief Run the app.
             */
            void run() {
                spdlog::info("App started.");
                spdlog::debug("Run start up systems.");
                run_start_up_systems();
                spdlog::debug("End start up system commands.");
                end_commands();
                do {
                    spdlog::debug("Run state change systems.");
                    run_state_change_systems();
                    spdlog::debug("Update states.");
                    update_states();
                    spdlog::debug("Run update systems.");
                    run_update_systems();
                    end_commands();
                    spdlog::debug("Run render systems.");
                    run_render_systems();
                    end_commands();
                    spdlog::debug("Tick events and clear out-dated events.");
                    tick_events();
                } while (m_loop_enabled && !run_system_v(std::function(check_exit)));
            }

            template <typename T>
            void load_runner() {
                auto runner = std::make_shared<SystemRunner>(this, m_pool);
                for (auto& node : m_systems) {
                    auto& scheduler = node->scheduler;
                    if (scheduler != nullptr && dynamic_cast<T*>(scheduler.get()) != NULL) {
                        runner->add_system(node);
                    }
                }
                m_runners.insert(std::make_pair(typeid(T).hash_code(), runner));
            }

            template <typename T>
            void prepare_runner() {
                m_runners[typeid(T).hash_code()]->prepare();
            }

            template <typename T>
            void run_runner() {
                m_runners[typeid(T).hash_code()]->run();
                m_runners[typeid(T).hash_code()]->wait();
                m_runners[typeid(T).hash_code()]->reset();
                end_commands();
            }

            /*! @brief Run the app in parallel.
             */
            void run_parallel() {
                spdlog::info("App started.");
                spdlog::info("Loading systems runners.");
                // load systems
                spdlog::debug("Load pre-startup systems.");
                load_runner<PreStartup>();
                spdlog::debug("Load start up systems.");
                load_runner<Startup>();
                spdlog::debug("Load post-startup systems.");
                load_runner<PostStartup>();
                spdlog::debug("Load state change systems.");
                load_runner<OnStateChange>();
                spdlog::debug("Load pre-update systems.");
                load_runner<PreUpdate>();
                spdlog::debug("Load update systems.");
                load_runner<Update>();
                spdlog::debug("Load post-update systems.");
                load_runner<PostUpdate>();
                spdlog::debug("Load pre-render systems.");
                load_runner<PreRender>();
                spdlog::debug("Load render systems.");
                load_runner<Render>();
                spdlog::debug("Load post-render systems.");
                load_runner<PostRender>();
                spdlog::info("Loading done.");

                prepare_runner<PreStartup>();
                prepare_runner<Startup>();
                prepare_runner<PostStartup>();
                spdlog::debug("Run pre-startup systems.");
                run_runner<PreStartup>();
                spdlog::debug("Run start up systems.");
                run_runner<Startup>();
                spdlog::debug("Run post-startup systems.");
                run_runner<PostStartup>();

                do {
                    spdlog::debug("Prepare runners.");
                    prepare_runner<OnStateChange>();
                    prepare_runner<PreUpdate>();
                    prepare_runner<Update>();
                    prepare_runner<PostUpdate>();
                    prepare_runner<PreRender>();
                    prepare_runner<Render>();
                    prepare_runner<PostRender>();
                    spdlog::debug("End runners preparation.");

                    spdlog::debug("Run state change systems");
                    run_runner<OnStateChange>();

                    spdlog::debug("Update states.");
                    update_states();

                    spdlog::debug("Run pre-update systems.");
                    run_runner<PreUpdate>();

                    spdlog::debug("Run update systems.");
                    run_runner<Update>();

                    spdlog::debug("Run post-update systems.");
                    run_runner<PostUpdate>();

                    spdlog::debug("Run pre-render systems.");
                    run_runner<PreRender>();

                    spdlog::debug("Run render systems.");
                    run_runner<Render>();

                    spdlog::debug("Run post-render systems.");
                    run_runner<PostRender>();

                    spdlog::debug("Tick events and clear out-dated events.");
                    tick_events();
                } while (m_loop_enabled && !run_system_v(std::function(check_exit)));
                spdlog::info("App terminated.");
            }
        };

        class LoopPlugin : public Plugin {
           public:
            void build(App& app) override { app.enable_loop(); }
        };
    }  // namespace entity
}  // namespace pixel_engine