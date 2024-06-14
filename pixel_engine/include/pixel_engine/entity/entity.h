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
            double avg_reach_time = 1;

            SystemNode(std::shared_ptr<Scheduler> scheduler, std::shared_ptr<BasicSystem<void>> system,
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
            SystemRunner(App* app) : app(app), ignore_scheduler(false), pool(4) {}
            SystemRunner(App* app, bool ignore_scheduler) : app(app), ignore_scheduler(ignore_scheduler), pool(4) {}
            SystemRunner(const SystemRunner& other) : pool(4) {
                app = other.app;
                ignore_scheduler = other.ignore_scheduler;
                tails = other.tails;
            }
            void run();
            void add_system(std::shared_ptr<SystemNode> node) { systems_all.push_back(node); }
            void prepare();
            void reset() { futures.clear(); }
            void wait() { pool.wait(); }
            void sort_time() {
                std::sort(systems_all.begin(), systems_all.end(),
                          [](const auto& a, const auto& b) { return a->time_to_reach() < b->time_to_reach(); });
            }
            void sort_depth() {
                std::sort(systems_all.begin(), systems_all.end(),
                          [](const auto& a, const auto& b) { return a->user_before_depth() < b->user_before_depth(); });
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
            BS::thread_pool pool;
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
                            cv.wait(lk, [&] { return tasks_in <= pool.get_thread_count(); });
                        }
                        tasks_in++;
                        if (ignore_scheduler) {
                            futures[next] = pool.submit_task([&, next] {
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
                            futures[next] = pool.submit_task([&, app, next] {
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

        template <typename T, typename U>
        struct contrary {
            const static bool value = false;
        };

        template <typename T, typename U>
        struct contrary<Resource<T>, Resource<U>> {
            const static bool value = resource_access_contrary<Resource<T>, Resource<U>>::value;
        };

        template <typename T, typename U>
        struct contrary<EventReader<T>, EventReader<U>> {
            const static bool value = event_access_contrary<EventReader<T>, EventReader<U>>::value;
        };

        template <typename T, typename U>
        struct contrary<EventWriter<T>, EventWriter<U>> {
            const static bool value = event_access_contrary<EventWriter<T>, EventWriter<U>>::value;
        };

        template <typename T, typename U>
        struct contrary<EventReader<T>, EventWriter<U>> {
            const static bool value = event_access_contrary<EventReader<T>, EventWriter<U>>::value;
        };

        template <typename T, typename U>
        struct contrary<EventWriter<T>, EventReader<U>> {
            const static bool value = event_access_contrary<EventWriter<T>, EventReader<U>>::value;
        };

        template <typename... T, typename... U>
        struct contrary<Query<T...>, Query<U...>> {
            const static bool value = queries_contrary<Query<T...>, Query<U...>>::value();
        };

        template <>
        struct contrary<Command, Command> {
            const static bool value = true;
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

        class App {
            friend class LoopPlugin;

           protected:
            bool m_loop_enabled = false;
            entt::registry m_registry;
            std::unordered_map<entt::entity, std::set<entt::entity>> m_entity_relation_tree;
            std::unordered_map<size_t, std::any> m_resources;
            std::unordered_map<size_t, std::deque<Event>> m_events;
            std::vector<Command> m_existing_commands;
            std::vector<std::unique_ptr<BasicSystem<void>>> m_state_update;
            std::vector<std::shared_ptr<SystemNode>> m_systems;
            std::unordered_map<size_t, std::shared_ptr<Plugin>> m_plugins;
            std::unordered_map<size_t, std::shared_ptr<SystemRunner>> m_runners;

            void enable_loop() { m_loop_enabled = true; }
            void disable_loop() { m_loop_enabled = false; }

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
                static_assert(std::same_as<T, Command> || is_template_of<Query, T>::value ||
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
                } else if constexpr (is_template_of<Query, T>::value) {
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
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, std::function<void(Args...)> func,
                            std::shared_ptr<SystemNode>* node = nullptr, before befores = before(),
                            after afters = after(), std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
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
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, void (*func)(Args...), std::shared_ptr<SystemNode>* node = nullptr,
                            before befores = before(), after afters = after()) {
                return add_system(scheduler, std::function<void(Args...)>(func), node, befores, afters);
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
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, std::function<void(Args...)> func, std::shared_ptr<SystemNode>* node,
                            after afters, before befores = before(),
                            std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system(scheduler, func, node, befores, afters, conditions);
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
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, void (*func)(Args...), std::shared_ptr<SystemNode>* node, after afters,
                            before befores = before(), std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system(scheduler, std::function<void(Args...)>(func), node, befores, afters, conditions);
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
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, std::function<void(Args...)> func, after afters, before befores = before(),
                            std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system(scheduler, func, nullptr, befores, afters, conditions);
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
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, void (*func)(Args...), after afters, before befores = before(),
                            std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system(scheduler, std::function<void(Args...)>(func), nullptr, befores, afters, conditions);
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
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, std::function<void(Args...)> func, before befores, after afters = after(),
                            std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system(scheduler, func, nullptr, befores, afters, conditions);
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
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, void (*func)(Args...), before befores, after afters = after(),
                            std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system(scheduler, std::function<void(Args...)>(func), nullptr, befores, afters, conditions);
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
            template <typename Sch, typename... Args>
            App& add_system_main(Sch scheduler, std::function<void(Args...)> func,
                                 std::shared_ptr<SystemNode>* node = nullptr, before befores = before(),
                                 after afters = after(),
                                 std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                std::shared_ptr<SystemNode> new_node =
                    std::make_shared<SystemNode>(std::make_shared<Sch>(scheduler),
                                                 std::make_shared<System<Args...>>(System<Args...>(this, func)), true);
                new_node->conditions = conditions;
                for (auto& before_node : afters.nodes) {
                    if (before_node != nullptr) {
                        new_node->user_defined_before.insert(before_node);
                    }
                }
                for (auto& after_node : befores.nodes) {
                    if (after_node != nullptr) {
                        after_node->user_defined_before.insert(new_node);
                    }
                }
                if (node != nullptr) {
                    *node = new_node;
                }
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
            template <typename Sch, typename... Args>
            App& add_system_main(Sch scheduler, void (*func)(Args...), std::shared_ptr<SystemNode>* node = nullptr,
                                 before befores = before(), after afters = after(),
                                 std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system_main(scheduler, std::function<void(Args...)>(func), node, befores, afters,
                                       conditions);
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
            template <typename Sch, typename... Args>
            App& add_system_main(Sch scheduler, std::function<void(Args...)> func, std::shared_ptr<SystemNode>* node,
                                 after afters, before befores = before(),
                                 std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system_main(scheduler, func, node, befores, afters, conditions);
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
            template <typename Sch, typename... Args>
            App& add_system_main(Sch scheduler, void (*func)(Args...), std::shared_ptr<SystemNode>* node, after afters,
                                 before befores = before(),
                                 std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system_main(scheduler, std::function<void(Args...)>(func), node, befores, afters,
                                       conditions);
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
            template <typename Sch, typename... Args>
            App& add_system_main(Sch scheduler, std::function<void(Args...)> func, after afters,
                                 before befores = before(),
                                 std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system_main(scheduler, func, nullptr, befores, afters, conditions);
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
            template <typename Sch, typename... Args>
            App& add_system_main(Sch scheduler, void (*func)(Args...), after afters, before befores = before(),
                                 std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system_main(scheduler, std::function<void(Args...)>(func), nullptr, befores, afters,
                                       conditions);
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
            template <typename Sch, typename... Args>
            App& add_system_main(Sch scheduler, std::function<void(Args...)> func, before befores,
                                 after afters = after(),
                                 std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system_main(scheduler, func, nullptr, befores, afters, conditions);
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
            template <typename Sch, typename... Args>
            App& add_system_main(Sch scheduler, void (*func)(Args...), before befores, after afters = after(),
                                 std::unordered_set<std::shared_ptr<condition>> conditions = {}) {
                return add_system_main(scheduler, std::function<void(Args...)>(func), nullptr, befores, afters,
                                       conditions);
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
                auto runner = std::make_shared<SystemRunner>(this);
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
                // add start up systems
                spdlog::debug("Load start up systems.");
                load_runner<Startup>();
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

                prepare_runner<Startup>();
                spdlog::debug("Run start up systems.");
                run_runner<Startup>();

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