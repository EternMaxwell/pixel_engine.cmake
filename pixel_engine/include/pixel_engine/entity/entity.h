﻿#pragma once

#include <any>
#include <deque>
#include <entt/entity/registry.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <pfr.hpp>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>

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
            for (auto& e : exit_events.read()) {
                return true;
            }
            return false;
        }

        void exit_app(EventWriter<AppExit> exit_events) {
            exit_events.write(AppExit{});
        }

        class App {
            friend class LoopPlugin;

           protected:
            bool m_loop_enabled = false;
            entt::registry m_registry;
            std::unordered_map<entt::entity, std::set<entt::entity>>
                m_entity_relation_tree;
            std::unordered_map<size_t, std::any> m_resources;
            std::unordered_map<size_t, std::deque<std::any>> m_events;
            std::vector<Command> m_existing_commands;
            std::vector<std::tuple<std::unique_ptr<Scheduler>,
                                   std::unique_ptr<BasicSystem<void>>,
                                   std::unique_ptr<BasicSystem<bool>>>>
                m_systems;

            void enable_loop() { m_loop_enabled = true; }
            void disable_loop() { m_loop_enabled = false; }

            template <typename T, typename... Args, typename Tuple,
                      std::size_t... I>
            T process(T (*func)(Args...), Tuple const& tuple,
                      std::index_sequence<I...>) {
                return func(std::get<I>(tuple)...);
            }

            template <typename T, typename... Args, typename Tuple>
            T process(T (*func)(Args...), Tuple const& tuple) {
                return process(
                    func, tuple,
                    std::make_index_sequence<std::tuple_size<Tuple>::value>());
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
                    if (m_app.m_resources.find(typeid(Arg).hash_code()) !=
                        m_app.m_resources.end()) {
                        return Resource<Arg>(
                            &m_app.m_resources[typeid(Arg).hash_code()]);
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

                EventWriter<Arg> value() {
                    return EventWriter<Arg>(
                        &m_app.m_events[typeid(Arg).hash_code()]);
                }
            };

            template <typename T>
            struct get_event_reader {};

            template <template <typename...> typename Template, typename Arg>
            struct get_event_reader<Template<Arg>> {
                friend class App;
                friend class EventReader<Arg>;

               private:
                App& m_app;

               public:
                get_event_reader(App& app) : m_app(app) {}

                EventReader<Arg> value() {
                    return EventReader<Arg>(
                        &m_app.m_events[typeid(Arg).hash_code()]);
                }
            };

            /*!
             * @brief This is where the systems get their parameters by
             * type. All possible type should be handled here.
             */
            template <typename T, typename... Args>
            std::tuple<T, Args...> get_values() {
                static_assert(std::same_as<T, Command> ||
                                  is_template_of<Query, T>::value ||
                                  is_template_of<Resource, T>::value ||
                                  is_template_of<EventReader, T>::value ||
                                  is_template_of<EventWriter, T>::value,
                              "an illegal argument type is used in systems.");
                if constexpr (std::same_as<T, Command>) {
                    if constexpr (sizeof...(Args) > 0) {
                        Command cmd = command();
                        m_existing_commands.push_back(cmd);
                        return std::tuple_cat(std::make_tuple(cmd),
                                              get_values<Args...>());
                    } else {
                        return std::make_tuple(command());
                    }
                } else if constexpr (is_template_of<Query, T>::value) {
                    T query(m_registry);
                    if constexpr (sizeof...(Args) > 0) {
                        return std::tuple_cat(std::make_tuple(query),
                                              get_values<Args...>());
                    } else {
                        return std::make_tuple(query);
                    }
                } else if constexpr (is_template_of<Resource, T>::value) {
                    T res = get_resource<T>(*this).value();
                    if constexpr (sizeof...(Args) > 0) {
                        return std::tuple_cat(std::make_tuple(res),
                                              get_values<Args...>());
                    } else {
                        return std::make_tuple(res);
                    }
                } else if constexpr (is_template_of<EventReader, T>::value) {
                    T reader = get_event_reader<T>(*this).value();
                    if constexpr (sizeof...(Args) > 0) {
                        return std::tuple_cat(std::make_tuple(reader),
                                              get_values<Args...>());
                    } else {
                        return std::make_tuple(reader);
                    }
                } else if constexpr (is_template_of<EventWriter, T>::value) {
                    T writer = get_event_writer<T>(*this).value();
                    if constexpr (sizeof...(Args) > 0) {
                        return std::tuple_cat(std::make_tuple(writer),
                                              get_values<Args...>());
                    } else {
                        return std::make_tuple(writer);
                    }
                }
            }

           public:
            /*! @brief Get the registry.
             * @return The registry.
             */
            const auto& registry() { return m_registry; }

            /*! @brief Get a command object on this app.
             * @return The command object.
             */
            Command command() {
                Command command(&m_registry, &m_entity_relation_tree,
                                &m_resources, &m_events);
                return command;
            }

            /*! @brief Run a system.
             * @tparam Args The types of the arguments for the system.
             * @param func The system to be run.
             * @return The App object itself.
             */
            template <typename... Args>
            App& run_system(void (*func)(Args...)) {
                auto args = get_values<Args...>();
                process(func, args);
                return *this;
            }

            /*! @brief Run a system with a condition.
             * @tparam Args1 The types of the arguments for the system.
             * @tparam Args2 The types of the arguments for the condition.
             * @param func The system to be run.
             * @param condition The condition for the system to be run.
             * @return The App object itself.
             */
            template <typename... Args1, typename... Args2>
            App& run_system(void (*func)(Args1...),
                            bool (*condition)(Args2...)) {
                auto args2 = get_values<Args2...>();
                if (process(condition, args2)) {
                    auto args1 = get_values<Args1...>();
                    process(func, args1);
                }
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
            T run_system_v(T (*func)(Args...)) {
                auto args = get_values<Args...>();
                return process(func, args);
            }

            /*! @brief Add a system.
             * @tparam Args The types of the arguments for the system.
             * @param func The system to be run.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args>
            App& add_system(Sch scheduler, void (*func)(Args...)) {
                m_systems.push_back(
                    std::make_tuple(std::make_unique<Sch>(scheduler),
                                    std::make_unique<System<Args...>>(
                                        System<Args...>(this, func)),
                                    nullptr));
                return *this;
            }

            /*! @brief Add a system with a condition.
             * @tparam Args1 The types of the arguments for the system.
             * @tparam Args2 The types of the arguments for the condition.
             * @param func The system to be run.
             * @param condition The condition for the system to be run.
             * @return The App object itself.
             */
            template <typename Sch, typename... Args1, typename... Args2>
            App& add_system(Sch scheduler, void (*func)(Args1...),
                            bool (*condition)(Args2...)) {
                m_systems.push_back(
                    std::make_tuple(std::make_unique<Sch>(scheduler),
                                    std::make_unique<System<Args1...>>(
                                        System<Args1...>(this, func)),
                                    std::make_unique<Condition<Args2...>>(
                                        Condition<Args2...>(this, condition))));
                return *this;
            }

            /*! @brief Add a plugin.
             * @param plugin The plugin to be added.
             * @return The App object itself.
             */
            template <typename T,
                      std::enable_if_t<std::is_base_of_v<Plugin, T>>* = nullptr>
            App& add_plugin(T plugin) {
                plugin.build(*this);
                return *this;
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

            /*! @brief Run the app.
             */
            void run() {
                // run start up systems
                for (auto& [scheduler, system, condition] : m_systems) {
                    if (scheduler != nullptr &&
                        dynamic_cast<Startup*>(scheduler.get()) != NULL &&
                        scheduler->should_run()) {
                        if (condition == nullptr || condition->run()) {
                            system->run();
                        }
                    }
                }

                // run update and render systems
                while (m_loop_enabled && !run_system_v(check_exit)) {
                    for (auto& [scheduler, system, condition] : m_systems) {
                        if (scheduler != nullptr &&
                            dynamic_cast<Update*>(scheduler.get()) != NULL &&
                            scheduler->should_run()) {
                            if (condition == nullptr || condition->run()) {
                                system->run();
                            }
                        }
                    }

                    for (auto& [scheduler, system, condition] : m_systems) {
                        if (scheduler != nullptr &&
                            dynamic_cast<Render*>(scheduler.get()) != NULL &&
                            scheduler->should_run()) {
                            if (condition == nullptr || condition->run()) {
                                system->run();
                            }
                        }
                    }
                }
            }
        };

        class LoopPlugin : Plugin {
           public:
            void build(App& app) override { app.enable_loop(); }
        };
    }  // namespace entity
}  // namespace pixel_engine