﻿#pragma once

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
            std::shared_ptr<Scheduler> scheduler;
            std::shared_ptr<BasicSystem<void>> system;
            std::vector<std::shared_ptr<SystemNode>> user_defined_before;
            std::vector<std::shared_ptr<SystemNode>> app_generated_before;

            SystemNode(std::shared_ptr<Scheduler> scheduler, std::shared_ptr<BasicSystem<void>> system)
                : scheduler(scheduler), system(system) {}

            std::tuple<std::shared_ptr<Scheduler>, std::shared_ptr<BasicSystem<void>>> to_tuple() {
                return std::make_tuple(scheduler, system);
            }
        };

        typedef std::shared_ptr<SystemNode> Node;

        struct SystemRunner {
           private:
            App* const app;
            const bool ignore_scheduler;

           public:
            SystemRunner(App* app) : app(app), ignore_scheduler(false) {}
            SystemRunner(App* app, bool ignore_scheduler) : app(app), ignore_scheduler(ignore_scheduler) {}
            void run();
            void add_system(std::shared_ptr<SystemNode> node) { tails.insert(node); }
            void prepare();
            void reset() { futures.clear(); }
            void wait() { pool.wait(); }

           private:
            std::unordered_set<std::shared_ptr<SystemNode>> tails;
            std::unordered_map<std::shared_ptr<SystemNode>, std::future<void>> futures;
            BS::thread_pool pool;
            std::shared_ptr<SystemNode> should_run(const std::shared_ptr<SystemNode>& sys);
            std::shared_ptr<SystemNode> get_next();
            bool done();
            bool all_load();
        };

        void SystemRunner::run() {
            std::atomic<int> tasks_in = 0;
            std::condition_variable cv;
            std::mutex m;
            while (!all_load()) {
                auto next = get_next();
                if (next != nullptr) {
                    tasks_in++;
                    if (ignore_scheduler) {
                        futures[next] = pool.submit_task([next, &tasks_in, &m, &cv] {
                            next->system->run();
                            tasks_in--;
                            std::unique_lock<std::mutex> lk(m);
                            cv.notify_all();
                            return;
                        });
                    } else {
                        App* app = this->app;
                        futures[next] = pool.submit_task([app, next, &tasks_in, &m, &cv] {
                            if (next->scheduler->should_run(app)) {
                                next->system->run();
                            }
                            tasks_in--;
                            std::unique_lock<std::mutex> lk(m);
                            cv.notify_all();
                            return;
                        });
                    }
                }
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk, [&] { return tasks_in <= pool.get_thread_count(); });
            }
        }

        void SystemRunner::prepare() {
            for (auto& sys : tails) {
                for (auto& before : sys->user_defined_before) {
                    tails.erase(before);
                }
                for (auto& before : sys->app_generated_before) {
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
            SystemRunner m_state_change_runner;
            SystemRunner m_start_up_runner;
            SystemRunner m_update_runner;
            SystemRunner m_render_runner;

            void enable_loop() { m_loop_enabled = true; }
            void disable_loop() { m_loop_enabled = false; }

            template <typename T, typename... Args, typename Tuple, std::size_t... I>
            T process(std::function<T(Args...)> func, Tuple const& tuple, std::index_sequence<I...>) {
                return func(std::get<I>(tuple)...);
            }

            template <typename T, typename... Args, typename Tuple>
            T process(std::function<T(Args...)> func, Tuple const& tuple) {
                return process(func, tuple, std::make_index_sequence<std::tuple_size<Tuple>::value>());
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
                        state.value().just_created = false;
                        state.value().m_state = state_next.value().m_state;
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
            App()
                : m_state_change_runner(this),
                  m_start_up_runner(this, true),
                  m_update_runner(this, true),
                  m_render_runner(this, true) {}

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
                process(func, get_values<Args...>());
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
                return process(func, get_values<Args...>());
            }

            template <typename Ret1, typename... Args1, typename Ret2, typename... Args2>
            const static bool systems_contrary(const std::function<Ret1(Args1...)>& func1,
                                               const std::function<Ret2(Args2...)>& func2) {
                if constexpr (sizeof...(Args1) == 0 || sizeof...(Args2) == 0) {
                    return false;
                }
                return (arg_contrary_to_args<Args1, Args2...>() || ...);
            }

            template <typename Arg, typename... Args>
            const static bool arg_contrary_to_args() {
                return (contrary<Arg, Args>::value || ...);
            }

            template <typename Sch, typename... Args>
            void configure_app_generated_before(std::shared_ptr<SystemNode>& node) {
                for (auto& sys : m_systems) {
                    if (sys->scheduler != nullptr && dynamic_cast<Sch*>(sys->scheduler.get()) != NULL) {
                        if (sys->system->contrary_to(node->system)) {
                            spdlog::debug("add app generated before.");
                            node->app_generated_before.push_back(sys);
                        }
                    }
                }
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, void (*func)(Args...)) {
                std::shared_ptr<SystemNode> node = std::make_shared<SystemNode>(
                    std::make_shared<Sch>(scheduler), std::make_shared<System<Args...>>(System<Args...>(this, func)));
                configure_app_generated_before<Sch>(node);
                m_systems.push_back(node);
                return *this;
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args, typename... Nods>
            App& add_system(Sch scheduler, void (*func)(Args...), std::shared_ptr<SystemNode>& systems_before_this,
                            Nods... nodes) {
                std::shared_ptr<SystemNode> node = std::make_shared<SystemNode>(
                    std::make_shared<Sch>(scheduler), std::make_shared<System<Args...>>(System<Args...>(this, func)));
                std::shared_ptr<SystemNode> before[] = {systems_before_this, nodes...};
                for (auto& before_node : before) {
                    if (before_node != nullptr) {
                        node->user_defined_before.push_back(before_node);
                    }
                }
                configure_app_generated_before<Sch>(node);
                m_systems.push_back(node);
                return *this;
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @param node The node this system will be in.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args, typename... Nods>
            App& add_system(Sch scheduler, void (*func)(Args...), std::shared_ptr<SystemNode>* node,
                            std::shared_ptr<SystemNode>& systems_before_this, Nods... nodes) {
                std::shared_ptr<SystemNode> new_node = std::make_shared<SystemNode>(
                    std::make_shared<Sch>(scheduler), std::make_shared<System<Args...>>(System<Args...>(this, func)));
                std::shared_ptr<SystemNode> before[] = {systems_before_this, nodes...};
                for (auto& before_node : before) {
                    if (before_node != nullptr) {
                        new_node->user_defined_before.push_back(before_node);
                    }
                }
                if (node != nullptr) {
                    *node = new_node;
                }
                configure_app_generated_before<Sch>(new_node);
                m_systems.push_back(new_node);
                return *this;
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param scheduler The scheduler for the system.
             * @param func The system to be run.
             * @param node The node this system will be in.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, void (*func)(Args...), std::shared_ptr<SystemNode>* node) {
                std::shared_ptr<SystemNode> new_node = std::make_shared<SystemNode>(
                    std::make_shared<Sch>(scheduler), std::make_shared<System<Args...>>(System<Args...>(this, func)));
                if (node != nullptr) {
                    *node = new_node;
                }
                configure_app_generated_before<Sch>(new_node);
                m_systems.push_back(new_node);
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

            /*! @brief Run the app in parallel.
             */
            void run_parallel() {
                spdlog::info("App started.");
                spdlog::info("Prepare systems.");
                // add start up systems
                spdlog::debug("Prepare start up systems.");
                for (auto& node : m_systems) {
                    auto& scheduler = node->scheduler;
                    if (scheduler != nullptr && dynamic_cast<Startup*>(scheduler.get()) != NULL) {
                        m_start_up_runner.add_system(node);
                    }
                }
                m_start_up_runner.prepare();
                spdlog::debug("Prepare state change systems.");
                for (auto& node : m_systems) {
                    auto& scheduler = node->scheduler;
                    if (scheduler != nullptr && dynamic_cast<OnStateChange*>(scheduler.get()) != NULL) {
                        m_state_change_runner.add_system(node);
                    }
                }
                m_state_change_runner.prepare();
                spdlog::debug("Prepare update systems.");
                for (auto& node : m_systems) {
                    auto& scheduler = node->scheduler;
                    if (scheduler != nullptr && dynamic_cast<Update*>(scheduler.get()) != NULL) {
                        m_update_runner.add_system(node);
                    }
                }
                m_update_runner.prepare();
                spdlog::debug("Prepare render systems.");
                for (auto& node : m_systems) {
                    auto& scheduler = node->scheduler;
                    if (scheduler != nullptr && dynamic_cast<Render*>(scheduler.get()) != NULL) {
                        m_render_runner.add_system(node);
                    }
                }
                m_render_runner.prepare();
                spdlog::info("Preparation done.");

                spdlog::debug("Run start up systems.");
                m_start_up_runner.run();
                m_start_up_runner.wait();
                m_start_up_runner.reset();
                end_commands();

                do {
                    spdlog::debug("Run state change systems");
                    m_state_change_runner.run();
                    m_state_change_runner.wait();
                    m_state_change_runner.reset();
                    end_commands();

                    spdlog::debug("Update states.");
                    update_states();

                    spdlog::debug("Run update systems.");
                    m_update_runner.run();
                    m_update_runner.wait();
                    m_update_runner.reset();
                    end_commands();

                    spdlog::debug("Run render systems.");
                    m_render_runner.run();
                    m_render_runner.wait();
                    m_render_runner.reset();
                    end_commands();

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