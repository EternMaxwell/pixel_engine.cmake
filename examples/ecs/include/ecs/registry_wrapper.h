#pragma once

#include <entt/entity/registry.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <tuple>

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
                    relation_tree)
            : m_registry(registry), m_parent_tree(relation_tree) {}
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
            std::cout << "despawning " << static_cast<int>(entity) << std::endl;
            m_registry.destroy(entity);

            for (auto child : m_parent_tree[entity]) {
                std::cout << "despawning child " << static_cast<int>(child)
                          << std::endl;
                m_registry.destroy(child);
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
            return *(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                         .each()
                         .begin());
        }
    };

    class App {
       private:
        entt::registry m_registry;
        std::unordered_map<entt::entity, std::set<entt::entity>>
            m_entity_relation_tree;

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

        /*!
         * @brief This is where the systems get their parameters by type.
         * All possible type should be handled here.
         */
        template <typename T, typename... Args>
        std::tuple<T, Args...> get_values() {
            static_assert(
                std::same_as<T, Command> || is_template_of<Query, T>::value,
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
            }
        }

       public:
        const auto& registry() { return m_registry; }
        auto command() {
            Command command(m_registry, m_entity_relation_tree);
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

        template <template <typename...> typename Q, typename... Qus,
                  typename... Exs>
        Query<std::tuple<Qus...>, std::tuple<Exs...>> get_query(
            std::tuple<Qus...> q, std::tuple<Exs...>) {
            Query<std::tuple<Qus...>, std::tuple<Exs...>> query(m_registry);
            return query;
        }

        void run() {}
    };
}  // namespace ecs_trial