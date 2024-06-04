#pragma once

#include <any>
#include <deque>
#include <entt/entity/registry.hpp>
#include <pfr.hpp>
#include <set>
#include <tuple>
#include <unordered_map>

#include "pixel_engine/entity/event.h"

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

        struct Bundle {};

        template <typename T>
        struct is_bundle {
            static constexpr bool value() {
                bool is_bundle = false;
                bool* target = &is_bundle;
                pfr::for_each_field(T(), [&](const auto& field) {
                    if constexpr (std::is_same_v<const Bundle&,
                                                 decltype(field)>) {
                        *target = true;
                    }
                });
                return is_bundle;
            }
        };

        struct internal {
            template <typename T, typename... Args>
            static void emplace_internal(entt::registry* registry,
                                         entt::entity entity, T arg,
                                         Args... args) {
                if constexpr (is_template_of<std::tuple, T>::value) {
                    emplace_internal_tuple(registry, entity, &arg);
                } else if constexpr (is_bundle<T>::value()) {
                    pfr::for_each_field(arg, [&](const auto& field) {
                        if (!std::is_same_v<Bundle, decltype(field)>) {
                            emplace_internal(registry, entity, field);
                        }
                    });
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
                                               std::tuple<Args...>* tuple,
                                               std::index_sequence<I...>) {
                emplace_internal(registry, entity, std::get<I>(*tuple)...);
            }

            template <typename... Args>
            static void emplace_internal_tuple(entt::registry* registry,
                                               entt::entity entity,
                                               std::tuple<Args...>* tuple) {
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

            /*! @brief Spawn an entity.
             * Note that the components to be added should not be type that is
             * inherited or a reference.
             * @brief Accepted types are:
             * @brief - Any pure struct.
             * @brief - std::tuple<Args...> where Args... are pure structs.
             * @brief - A pure struct that contains a Bundle, which means it is
             * a bundle, and all its fields will be components.
             * @tparam Args The types of the components to be added to the
             * child entity.
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
            std::unordered_map<size_t, std::deque<Event>>* m_events;

           public:
            Command(entt::registry* registry,
                    std::unordered_map<entt::entity, std::set<entt::entity>>*
                        relation_tree,
                    std::unordered_map<size_t, std::any>* resources,
                    std::unordered_map<size_t, std::deque<Event>>* events)
                : m_registry(registry),
                  m_parent_tree(relation_tree),
                  m_resources(resources),
                  m_events(events) {}

            /*! @brief Spawn an entity.
             * Note that the components to be added should not be type that is
             * inherited or a reference.
             * @brief Accepted types are:
             * @brief - Any pure struct.
             * @brief - std::tuple<Args...> where Args... are pure structs.
             * @brief - A pure struct that contains a Bundle, which means it is
             * a bundle, and all its fields will be components.
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
             * If the resource already exists, nothing will happen.
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
             * If the resource does not exist, nothing will happen.
             * @tparam T The type of the resource.
             */
            template <typename T>
            void remove_resource() {
                m_resources->erase(typeid(T).hash_code());
            }

            /*! @brief Insert Resource using default values.
             * If the resource already exists, nothing will happen.
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

            /*! @brief Clear events of type T.
             * If the event does not exist nothing will happen.
             * @tparam T The type of the event.
             */
            template <typename T>
            void clear_events() {
                m_events->erase(typeid(T).hash_code());
            }

            /*! @brief Not figured out what this do and how to use it.
             */
            void end() {}
        };
    }  // namespace entity
}  // namespace pixel_engine