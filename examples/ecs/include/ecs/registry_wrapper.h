#pragma once

#include <entt/entity/registry.hpp>
#include <map>
#include <set>
#include <tuple>

namespace ecs_trial {
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

        auto& registry() { return m_registry; }

        void end() {}
    };

    class App {
       private:
        entt::registry m_registry;
        std::unordered_map<entt::entity, std::set<entt::entity>>
            m_entity_relation_tree;

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
            for (int i = 0; i < std::tuple_size_v<argument_type>; i++) {
                std::cout << i << std::endl;
            }

            std::tuple<> value;
            for (int i = 0; i < sizeof...(Args); i++) {
                if (std::same_as<std::tuple_element_t<i, argument_type>,
                                 Command>) {
                }
            }

            return *this;
        }

        void run() {}
    };
}  // namespace ecs_trial