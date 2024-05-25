#pragma once

#include <any>
#include <entt/entity/registry.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>

namespace ecs_trial {
    template <template <typename...> typename Template, typename T>
    struct is_template_of {
        static const bool value = false;
    };

    template <template <typename...> typename Template, typename... Args>
    struct is_template_of<Template, Template<Args...>> {
        static const bool value = true;
    };

    class Command {
       private:
        entt::registry& m_registry;
        std::unordered_map<entt::entity, std::set<entt::entity>>& m_parent_tree;
        std::unordered_map<size_t, std::any>& m_resources;

        template <typename T, typename... Args>
        void emplace_internal(entt::entity entity, T arg, Args... args) {
            m_registry.emplace<T>(entity, arg);
            if constexpr (sizeof...(Args) > 0) {
                emplace_internal(entity, args...);
            }
        }

       public:
        Command(entt::registry& registry,
                std::unordered_map<entt::entity, std::set<entt::entity>>&
                    relation_tree,
                std::unordered_map<size_t, std::any>& resources)
            : m_registry(registry),
              m_parent_tree(relation_tree),
              m_resources(resources) {}
        template <typename... Args>
        auto spawn(Args... args) {
            auto entity = m_registry.create();
            emplace_internal(entity, args...);
            return entity;
        }

        template <typename... Args>
        auto with_child(const entt::entity& entity, Args... args) {
            auto child = spawn(args...);
            m_parent_tree[entity].insert(child);
            return child;
        }

        void despawn(const entt::entity& entity) { m_registry.destroy(entity); }

        void despawn_recurse(const entt::entity& entity) {
            m_registry.destroy(entity);
            for (auto child : m_parent_tree[entity]) {
                m_registry.destroy(child);
            }
            m_parent_tree.erase(entity);
        }

        template <typename T>
        void insert_resource(T res) {
            if (m_resources.find(typeid(T).hash_code()) == m_resources.end()) {
                m_resources.insert({typeid(T).hash_code(), res});
            }
        }

        template <typename T>
        void remove_resource() {
            m_resources.erase(typeid(T).hash_code());
        }

        template <typename T>
        void init_resource() {
            if (m_resources.find(typeid(T).hash_code()) == m_resources.end()) {
                auto res = std::make_any<T>();
                m_resources.insert({typeid(T).hash_code(), res});
            }
        }

        void end() {}
    };

    template <typename Que, typename Exclude>
    class Query {};

    template <typename... Qus, typename... Exs>
    class Query<std::tuple<Qus...>, std::tuple<Exs...>> {
       private:
        entt::registry& registry;

       public:
        Query(entt::registry& registry) : registry(registry) {}

        auto iter() {
            return registry.view<Qus...>(entt::exclude_t<Exs...>{}).each();
        }

        auto single() {
            auto start = *(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                               .each()
                               .begin());
            if (registry.view<Qus...>(entt::exclude_t<Exs...>{})
                    .each()
                    .begin() !=
                registry.view<Qus...>(entt::exclude_t<Exs...>{}).each().end()) {
                return std::optional(start);
            } else {
                return std::optional<decltype(start)>{};
            }
        }
    };

    template <typename ResT>
    class Resource {
       private:
        std::any* const m_res;

       public:
        Resource(std::any* resource) : m_res(resource) {}
        Resource() : m_res(nullptr) {}

        bool has_value() { return m_res != nullptr && m_res->has_value(); }
        ResT& value() { return std::any_cast<ResT&>(*m_res); }
    };

    class App {
       private:
        entt::registry m_registry;
        std::unordered_map<entt::entity, std::set<entt::entity>>
            m_entity_relation_tree;
        std::unordered_map<size_t, std::any> m_resources;

        template <typename... Args, typename Tuple, std::size_t... I>
        void process(void (*func)(Args...), Tuple const& tuple,
                     std::index_sequence<I...>) {
            func(std::get<I>(tuple)...);
        }

        template <typename... Args, typename Tuple>
        void process(void (*func)(Args...), Tuple const& tuple) {
            process(func, tuple,
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

        /*!
         * @brief This is where the systems get their parameters by type.
         * All possible type should be handled here.
         */
        template <typename T, typename... Args>
        std::tuple<T, Args...> get_values() {
            static_assert(std::same_as<T, Command> ||
                              is_template_of<Query, T>::value ||
                              is_template_of<Resource, T>::value,
                          "an illegal argument type is used in systems.");
            if constexpr (std::same_as<T, Command>) {
                if constexpr (sizeof...(Args) > 0) {
                    return std::tuple_cat(std::make_tuple(command()),
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
            }
        }

       public:
        const auto& registry() { return m_registry; }
        auto command() {
            Command command(m_registry, m_entity_relation_tree, m_resources);
            return command;
        }

        App& run_system_command(void (*func)(Command)) {
            func(command());
            return *this;
        }

        template <typename... Args>
        App& run_system(void (*func)(Args...)) {
            using argument_type = std::tuple<Args...>;
            auto args = get_values<Args...>();
            process(func, args);
            return *this;
        }

        void run() {}
    };
}  // namespace ecs_trial