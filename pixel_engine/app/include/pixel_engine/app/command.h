#pragma once

#include <unordered_map>
#include <unordered_set>

#include "tools.h"

namespace pixel_engine {
namespace app {
using namespace internal_components;

struct EntityCommand {
   private:
    entt::registry* const m_registry;
    const entt::entity m_entity;
    std::unordered_map<entt::entity, std::unordered_set<entt::entity>>*
        m_entity_tree;
    std::shared_ptr<std::vector<entt::entity>> m_despawns;
    std::shared_ptr<std::vector<entt::entity>> m_recursive_despawns;

   public:
    EntityCommand(
        entt::registry* registry,
        entt::entity entity,
        std::unordered_map<entt::entity, std::unordered_set<entt::entity>>*
            entity_tree,
        std::shared_ptr<std::vector<entt::entity>> despawns,
        std::shared_ptr<std::vector<entt::entity>> recursive_despawns
    );

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
    entt::entity spawn(Args&&... args) {
        auto child = m_registry->create();
        app_tools::registry_emplace(
            m_registry, child, Parent{.id = m_entity}, args...
        );
        (*m_entity_tree)[m_entity].insert(child);
        return child;
    }

    /*! @brief Emplace new components to the entity.
     * @tparam Args The types of the components to be added to the
     * entity.
     * @param args The components to be added to the entity.
     */
    template <typename... Args>
    void emplace(Args&&... args) {
        app_tools::registry_emplace(
            m_registry, m_entity, args...
        );
    }

    template <typename... Args>
    void erase() {
        app_tools::registry_erase<Args...>(m_registry, m_entity);
    }

    /*! @brief Despawn an entity.
     */
    void despawn();

    /*! @brief Despawn an entity and all its children.
     */
    void despawn_recurse();
};

struct Command {
   private:
    entt::registry* const m_registry;
    std::unordered_map<const type_info*, std::shared_ptr<void>>* m_resources;
    std::unordered_map<entt::entity, std::unordered_set<entt::entity>>*
        m_entity_tree;
    std::shared_ptr<std::vector<entt::entity>> m_despawns;
    std::shared_ptr<std::vector<entt::entity>> m_recursive_despawns;

   public:
    Command(
        entt::registry* registry,
        std::unordered_map<const type_info*, std::shared_ptr<void>>* resources,
        std::unordered_map<entt::entity, std::unordered_set<entt::entity>>*
            entity_tree
    );

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
    entt::entity spawn(Args&&... args) {
        auto entity = m_registry->create();
        app_tools::registry_emplace(m_registry, entity, args...);
        return entity;
    }

    /**
     * @brief Get the entity command for the entity.
     *
     * @param entity The entity id.
     * @return `EntityCommand` The entity command.
     */
    EntityCommand entity(entt::entity entity);

    /*! @brief Insert a resource.
     * If the resource already exists, nothing will happen.
     * @tparam T The type of the resource.
     * @tparam Args The types of the arguments to be passed to the
     * @param args The arguments to be passed to the constructor of T
     */
    template <typename T, typename... Args>
    void insert_resource(Args&&... args) {
        if (m_resources->find(&typeid(T)) == m_resources->end()) {
            m_resources->emplace(std::make_pair(
                &typeid(T),
                std::static_pointer_cast<void>(
                    std::make_shared<std::remove_reference_t<T>>(args...)
                )
            ));
        }
    }

    template <typename T>
    void insert_resource(T&& res) {
        if (m_resources->find(&typeid(T)) == m_resources->end()) {
            m_resources->emplace(std::make_pair(
                &typeid(T),
                std::static_pointer_cast<void>(
                    std::make_shared<std::remove_reference_t<T>>(std::move(res))
                )
            ));
        }
    }

    /*! @brief Remove a resource.
     * If the resource does not exist, nothing will happen.
     * @tparam T The type of the resource.
     */
    template <typename T>
    void remove_resource() {
        m_resources->erase(&typeid(T));
    }

    /*! @brief Insert Resource using default values.
     * If the resource already exists, nothing will happen.
     * @tparam T The type of the resource.
     */
    template <typename T>
    void init_resource() {
        if (m_resources->find(&typeid(T)) == m_resources->end()) {
            auto res = std::static_pointer_cast<void>(
                std::make_shared<std::remove_reference_t<T>>()
            );
            m_resources->insert({&typeid(T), res});
        }
    }

    void end();
};
}  // namespace app
}  // namespace pixel_engine
