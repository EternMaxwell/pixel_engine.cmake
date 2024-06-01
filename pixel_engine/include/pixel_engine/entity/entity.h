#pragma once

#include <any>
#include <deque>
#include <entt/entity/registry.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include "pixel_engine/entity/system.h"

namespace pixel_engine {
    namespace entity {
        template <template <typename...> typename Template, typename T>
        struct is_template_of {
            static const bool value = false;
        };

        template <template <typename...> typename Template, typename... Args>
        struct is_template_of<Template, Template<Args...>> {
            static const bool value = true;
        };

        template <typename Q1, typename Q2>
        struct queries_contrary {};

        struct internal {
            template <typename T, typename... Args>
            static void emplace_internal(entt::registry* registry,
                                         entt::entity entity, T arg,
                                         Args... args) {
                if constexpr (is_template_of<std::tuple, T>::value) {
                    emplace_internal_tuple(registry, entity, arg);
                } else {
                    registry->emplace<T>(entity, arg);
                }
                if constexpr (sizeof...(Args) > 0) {
                    emplace_internal(registry, entity, args...);
                }
            }

            template <typename... Args, size_t... I>
            static void emplace_internal_tuple(entt::registry* registry,
                                               entt::entity entity,
                                               std::tuple<Args...> tuple,
                                               std::index_sequence<I...>) {
                emplace_internal(registry, entity, std::get<I>(tuple)...);
            }

            template <typename... Args>
            static void emplace_internal_tuple(entt::registry* registry,
                                               entt::entity entity,
                                               std::tuple<Args...> tuple) {
                emplace_internal_tuple(
                    registry, entity, tuple,
                    std::make_index_sequence<sizeof...(Args)>());
            }
        };

        class EntityCommand {
           private:
            entt::registry* const m_registry;
            const entt::entity& m_entity;
            std::unordered_map<entt::entity, std::set<entt::entity>>* const
                m_parent_tree;

           public:
            EntityCommand(
                entt::registry* registry, const entt::entity& entity,
                std::unordered_map<entt::entity, std::set<entt::entity>>*
                    relation_tree)
                : m_registry(registry),
                  m_entity(entity),
                  m_parent_tree(relation_tree) {}

            /*! @brief Spawn a child entity.
             * @tparam Args The types of the components to be added to the child
             * entity.
             * @param args The components to be added to the child entity.
             * @return The child entity id.
             */
            template <typename... Args>
            auto spawn(Args... args) {
                auto child = m_registry->create();
                entity::internal::emplace_internal(m_registry, child, args...);
                (*m_parent_tree)[m_entity].insert(child);
                return child;
            }

            /*! @brief Despawn the entity.
             */
            void despawn() { m_registry->destroy(m_entity); }

            /*! @brief Despawn the entity and all its children.
             */
            void despawn_recurse() {
                m_registry->destroy(m_entity);
                for (auto child : (*m_parent_tree)[m_entity]) {
                    m_registry->destroy(child);
                }
                m_parent_tree->erase(m_entity);
            }
        };

        class Command {
           private:
            entt::registry* const m_registry;
            std::unordered_map<entt::entity, std::set<entt::entity>>* const
                m_parent_tree;
            std::unordered_map<size_t, std::any>* const m_resources;

           public:
            Command(entt::registry* registry,
                    std::unordered_map<entt::entity, std::set<entt::entity>>*
                        relation_tree,
                    std::unordered_map<size_t, std::any>* resources)
                : m_registry(registry),
                  m_parent_tree(relation_tree),
                  m_resources(resources) {}

            /*! @brief Spawn an entity.
             * @tparam Args The types of the components to be added to the
             * entity.
             * @param args The components to be added to the entity.
             * @return The entity id.
             */
            template <typename... Args>
            auto spawn(Args... args) {
                auto entity = m_registry->create();
                entity::internal::emplace_internal(m_registry, entity, args...);
                return entity;
            }

            /*! @brief Get the EntityCommand object for the entity.
             * @param entity The entity id.
             * @return The EntityCommand object for the entity.
             */
            auto entity(const entt::entity& entity) {
                return EntityCommand(m_registry, entity, m_parent_tree);
            }

            /*! @brief Insert a resource.
             * @tparam T The type of the resource.
             * @param res The resource to be inserted.
             */
            template <typename T>
            void insert_resource(T res) {
                if (m_resources->find(typeid(T).hash_code()) ==
                    m_resources->end()) {
                    m_resources->insert({typeid(T).hash_code(), res});
                }
            }

            /*! @brief Remove a resource.
             * @tparam T The type of the resource.
             */
            template <typename T>
            void remove_resource() {
                m_resources->erase(typeid(T).hash_code());
            }

            /*! @brief Insert Resource using default values.
             * @tparam T The type of the resource.
             */
            template <typename T>
            void init_resource() {
                if (m_resources->find(typeid(T).hash_code()) ==
                    m_resources->end()) {
                    auto res = std::make_any<T>();
                    m_resources->insert({typeid(T).hash_code(), res});
                }
            }

            /*! @brief Not figured out what this do and how to use it.
             */
            void end() {}
        };

        template <typename Que, typename Exclude>
        class Query {};

        template <typename... Qus, typename... Exs>
        class Query<std::tuple<Qus...>, std::tuple<Exs...>> {
           private:
            entt::registry& registry;
            decltype(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                         .each()) m_iter;

           public:
            Query(entt::registry& registry) : registry(registry) {
                m_iter =
                    registry.view<Qus...>(entt::exclude_t<Exs...>{}).each();
            }

            class iterator : public std::iterator<std::input_iterator_tag,
                                                  std::tuple<Qus...>> {
               private:
                decltype(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                             .each()) m_full;
                decltype(m_full.begin()) m_iter;

                template <size_t I, typename T, typename... Args>
                std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
                    if constexpr (sizeof...(Args) > I) {
                        return std::tuple_cat(
                            std::make_tuple(std::get<I>(tuple)),
                            remove_first<I++>(tuple));
                    } else {
                        return std::make_tuple(std::get<I>(tuple));
                    }
                }

                template <typename T, typename... Args>
                std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
                    return remove_first<1>(tuple);
                }

               public:
                iterator(decltype(m_full) full)
                    : m_full(full), m_iter(full.begin()) {}

                iterator(decltype(m_full) full, decltype(m_full.begin()) iter)
                    : m_full(full), m_iter(iter) {}
                iterator& operator++() {
                    m_iter++;
                    return *this;
                }
                iterator operator++(int) {
                    iterator tmp = *this;
                    ++(*this);
                    return tmp;
                }
                bool operator==(const iterator& rhs) const {
                    return m_iter == rhs.m_iter;
                }
                bool operator!=(const iterator& rhs) const {
                    return m_iter != rhs.m_iter;
                }
                std::tuple<Qus...> operator*() {
                    // remove the first element of the tuple and return the rest
                    return remove_first(*m_iter);
                }
                auto begin() { return iterator(m_full, m_full.begin()); }

                auto end() { return iterator(m_full, m_full.end()); }
            };

            /*! @brief Get the iterator for the query.
             * @return The iterator for the query.
             */
            auto iter() {
                return iterator(
                    registry.view<Qus...>(entt::exclude_t<Exs...>{}).each());
            }

            /*! @brief Get the single entity and requaired components.
             * @return An optional of a single tuple of entity and requaired
             * components.
             */
            auto single() {
                auto start = *(iter().begin());
                if (iter().begin() != iter().end()) {
                    return std::optional(start);
                } else {
                    return std::optional<decltype(start)>{};
                }
            }
        };

        template <typename... Qus, typename... Exs>
        class Query<std::tuple<entt::entity, Qus...>, std::tuple<Exs...>> {
           private:
            entt::registry& registry;
            decltype(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                         .each()) m_iter;

           public:
            Query(entt::registry& registry) : registry(registry) {
                m_iter =
                    registry.view<Qus...>(entt::exclude_t<Exs...>{}).each();
            }

            /*! @brief Get the iterator for the query.
             * @return The iterator for the query.
             */
            auto iter() {
                return registry.view<Qus...>(entt::exclude_t<Exs...>{}).each();
            }

            /*! @brief Get the single entity and requaired components.
             * @return An optional of a single tuple of entity and requaired
             * components.
             */
            auto single() {
                auto start = *(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                                   .each()
                                   .begin());
                if (registry.view<Qus...>(entt::exclude_t<Exs...>{})
                        .each()
                        .begin() !=
                    registry.view<Qus...>(entt::exclude_t<Exs...>{})
                        .each()
                        .end()) {
                    return std::optional(start);
                } else {
                    return std::optional<decltype(start)>{};
                }
            }
        };

        template <typename Qu1, typename Qu2, typename... Qus1,
                  typename... Qus2, typename... Exs1, typename... Exs2>
        struct queries_contrary<
            Query<std::tuple<Qu1, Qus1...>, std::tuple<Exs1...>>,
            Query<std::tuple<Qu2, Qus2...>, std::tuple<Exs2...>>> {
            template <typename T, typename... Args>
            bool args_all_const() {
                if constexpr (std::is_const_v<T>) {
                    if constexpr (sizeof...(Args) > 0) {
                        return args_all_const<Args...>();
                    } else {
                        return true;
                    }
                } else {
                    return false;
                }
            }

            template <typename T>
            static bool type_in_group() {
                return false;
            }

            template <typename T, typename First, typename... Args>
            static bool type_in_group() {
                if constexpr (std::is_same_v<std::remove_const_t<T>,
                                             std::remove_const_t<First>>) {
                    return true;
                } else {
                    if constexpr (sizeof...(Args) > 0) {
                        return type_in_group<T, Args...>();
                    } else {
                        return false;
                    }
                }
            }

            template <typename First, typename Second>
            struct first_tuple_contains_any_type_in_second_tuple {
                static const bool value = false;
            };

            template <typename... Args1, typename Args2First, typename... Args2>
            struct first_tuple_contains_any_type_in_second_tuple<
                std::tuple<Args1...>, std::tuple<Args2First, Args2...>> {
                bool value() {
                    if constexpr (sizeof...(Args1) == 0) {
                        return false;
                    } else if (type_in_group<std::remove_const_t<Args2First>,
                                             Args1...>()) {
                        return true;
                    } else {
                        if constexpr (sizeof...(Args2) > 0) {
                            return first_tuple_contains_any_type_in_second_tuple<
                                       std::tuple<Args1...>,
                                       std::tuple<Args2...>>{}
                                .value();
                        } else {
                            return false;
                        }
                    }
                }
            };

            template <typename First, typename Second>
            struct tuples_has_no_same_non_const_type {
                static const bool value = false;
            };

            template <typename... Args1, typename Args2First, typename... Args2>
            struct tuples_has_no_same_non_const_type<
                std::tuple<Args1...>, std::tuple<Args2First, Args2...>> {
                bool value() {
                    if constexpr (sizeof...(Args1) == 0) {
                        return true;
                    } else if ((!std::is_const_v<Args2First>)&&type_in_group<
                                   Args2First, Args1...>()) {
                        return false;
                    } else {
                        if constexpr (sizeof...(Args2) > 0) {
                            return tuples_has_no_same_non_const_type<
                                       std::tuple<Args1...>,
                                       std::tuple<Args2...>>{}
                                .value();
                        } else {
                            return true;
                        }
                    }
                }
            };

            bool value() {
                if constexpr ((!std::is_same_v<Qu1, entt::entity>)&&(
                                  !std::is_same_v<Qu2, entt::entity>)) {
                    // if Qus1 and Qus2 are all const return false
                    if (args_all_const<Qu1, Qus1..., Qu2, Qus2...>()) {
                        return false;
                    }

                    // if Exs1 contains any non-const Type in Qus2 return false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs2...>, std::tuple<Qu1, Qus1...>>{}
                            .value()) {
                        return false;
                    }

                    // if Exs2 contains any non-const Type in Qus1 return false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs1...>, std::tuple<Qu2, Qus2...>>{}
                            .value()) {
                        return false;
                    }

                    // if Qus1 and Qus2 has no same non-const type return false
                    if (tuples_has_no_same_non_const_type<
                            std::tuple<Qu1, Qus1...>,
                            std::tuple<Qu2, Qus2...>>{}
                            .value()) {
                        return false;
                    }
                } else if constexpr (std::is_same_v<Qu1, entt::entity>) {
                    // if Qus1 and Qus2 are all const return false
                    if (args_all_const<Qus1..., Qu2, Qus2...>()) {
                        return false;
                    }

                    // if Exs1 contains any non-const Type in Qus2 return false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs2...>, std::tuple<Qus1...>>{}
                            .value()) {
                        return false;
                    }

                    // if Exs2 contains any non-const Type in Qus1 return false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs1...>, std::tuple<Qu2, Qus2...>>{}
                            .value()) {
                        return false;
                    }

                    // if Qus1 and Qus2 has no same non-const type return false
                    if (tuples_has_no_same_non_const_type<
                            std::tuple<Qus1...>, std::tuple<Qu2, Qus2...>>{}
                            .value()) {
                        return false;
                    }
                } else if constexpr (std::is_same_v<Qu2, entt::entity>) {
                    // if Qus1 and Qus2 are all const return false
                    if (args_all_const<Qu1, Qus1..., Qus2...>()) {
                        return false;
                    }

                    // if Exs1 contains any non-const Type in Qus2 return false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs2...>, std::tuple<Qu1, Qus1...>>{}
                            .value()) {
                        return false;
                    }

                    // if Exs2 contains any non-const Type in Qus1 return false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs1...>, std::tuple<Qus2...>>{}
                            .value()) {
                        return false;
                    }

                    // if Qus1 and Qus2 has no same non-const type return false
                    if (tuples_has_no_same_non_const_type<
                            std::tuple<Qu1, Qus1...>, std::tuple<Qus2...>>{}
                            .value()) {
                        return false;
                    }
                }

                return true;
            }
        };

        template <typename ResT>
        class Resource {
           private:
            std::any* const m_res;

           public:
            Resource(std::any* resource) : m_res(resource) {}
            Resource() : m_res(nullptr) {}

            /*! @brief Check if the resource is const.
             * @return True if the resource is const, false otherwise.
             */
            constexpr bool is_const() {
                return std::is_const_v<std::remove_reference_t<ResT>>;
            }

            /*! @brief Check if the resource has a value.
             * @return True if the resource has a value, false otherwise.
             */
            bool has_value() { return m_res != nullptr && m_res->has_value(); }

            /*! @brief Get the value of the resource.
             * @return The value of the resource.
             */
            ResT& value() { return std::any_cast<ResT&>(*m_res); }
        };

        template <typename Evt>
        class EventWriter {
           private:
            std::deque<std::any>* const m_events;

           public:
            EventWriter(std::deque<std::any>* events) : m_events(events) {}

            /*! @brief Write an event.
             * @param evt The event to be written.
             */
            auto& write(Evt evt) {
                m_events->push_back(evt);
                return *this;
            }
        };

        template <typename Evt>
        class EventReader {
           private:
            std::deque<std::any>* const m_events;

           public:
            EventReader(std::deque<std::any>* events) : m_events(events) {}

            class event_iter
                : public std::iterator<std::input_iterator_tag, Evt> {
               private:
                std::deque<std::any>* m_events;
                std::deque<std::any>::iterator m_iter;

               public:
                event_iter(std::deque<std::any>* events,
                           std::deque<std::any>::iterator iter)
                    : m_events(events), m_iter(iter) {}
                event_iter& operator++() {
                    m_iter++;
                    return *this;
                }
                event_iter operator++(int) {
                    event_iter tmp = *this;
                    ++(*this);
                    return tmp;
                }
                bool operator==(const event_iter& rhs) const {
                    return m_iter == rhs.m_iter;
                }
                bool operator!=(const event_iter& rhs) const {
                    return m_iter != rhs.m_iter;
                }
                Evt& operator*() { return std::any_cast<Evt&>(*m_iter); }

                auto begin() { return event_iter(m_events, m_events->begin()); }

                auto end() { return event_iter(m_events, m_events->end()); }
            };

            /*! @brief Read an event.
             * @return The iterator for the events.
             */
            auto read() { return event_iter(m_events, m_events->begin()); }

            /*! @brief Check if the event queue is empty.
             * @return True if the event queue is empty, false otherwise.
             */
            bool empty() { return m_events->empty(); }

            /*! @brief Clear the event queue.
             */
            void clear() { m_events->clear(); }
        };

        class Scheduler {};

        class Plugin {
           public:
            virtual void build(App& app) = 0;
        };

        class App {
           protected:
            entt::registry m_registry;
            std::unordered_map<entt::entity, std::set<entt::entity>>
                m_entity_relation_tree;
            std::unordered_map<size_t, std::any> m_resources;
            std::unordered_map<size_t, std::deque<std::any>> m_events;
            std::vector<Command> m_existing_commands;
            std::vector<std::unique_ptr<BasicSystem<void>>> m_systems;
            std::unordered_map<std::unique_ptr<BasicSystem<void>>*,
                               std::unique_ptr<BasicSystem<bool>>>
                m_system_conditions;

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
             * @brief This is where the systems get their parameters by type.
             * All possible type should be handled here.
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
                                &m_resources);
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
            template <typename... Args>
            App& add_system(void (*func)(Args...)) {
                m_systems.push_back(std::make_unique<System<Args...>>(
                    System<Args...>(this, func)));
                return *this;
            }

            /*! @brief Add a system with a condition.
             * @tparam Args1 The types of the arguments for the system.
             * @tparam Args2 The types of the arguments for the condition.
             * @param func The system to be run.
             * @param condition The condition for the system to be run.
             * @return The App object itself.
             */
            template <typename... Args1, typename... Args2>
            App& add_system(void (*func)(Args1...),
                            bool (*condition)(Args2...)) {
                m_systems.push_back(std::make_unique<System<Args1...>>(
                    System<Args1...>(this, func)));
                m_system_conditions[&m_systems.back()] =
                    std::make_unique<Condition<Args2...>>(
                        Condition<Args2...>(this, condition));
                return *this;
            }

            /*! @brief Add a plugin.
             * @param plugin The plugin to be added.
             * @return The App object itself.
             */
            App& add_plugin(Plugin& plugin) {
                plugin.build(*this);
                return *this;
            }

            /*! @brief Add a plugin.
             * @param plugin The plugin to be added.
             * @return The App object itself.
             */
            App& add_plugin(Plugin* plugin) {
                plugin->build(*this);
                return *this;
            }

            /*! @brief Run the app.
             * Still need to figure out how to use this.
             */
            void run() {
                for (auto& system : m_systems) {
                    if (m_system_conditions.find(&system) !=
                        m_system_conditions.end()) {
                        if (m_system_conditions.at(&system)->run()) {
                            system->run();
                        }
                    } else {
                        system->run();
                    }
                }
            }
        };
    }  // namespace entity
}  // namespace pixel_engine