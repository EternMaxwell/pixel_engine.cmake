#pragma once

#include <sparsepp/spp.h>

#include <entt/entity/registry.hpp>

#include "tools.h"
#include "world.h"


namespace pixel_engine {
namespace app {
using namespace internal_components;

struct EntityCommand {
   private:
    entt::registry* const m_registry;
    const entt::entity m_entity;
    spp::sparse_hash_map<entt::entity, spp::sparse_hash_set<entt::entity>>*
        m_entity_tree;
    std::shared_ptr<std::vector<entt::entity>> m_despawns;
    std::shared_ptr<std::vector<entt::entity>> m_recursive_despawns;

   public:
    EntityCommand(
        World* world,
        entt::entity entity,
        std::shared_ptr<std::vector<entt::entity>> despawns,
        std::shared_ptr<std::vector<entt::entity>> recursive_despawns
    )
        : m_registry(&world->m_registry),
          m_entity(entity),
          m_entity_tree(&world->m_entity_tree),
          m_despawns(despawns),
          m_recursive_despawns(recursive_despawns) {}

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
        app_tools::registry_emplace(m_registry, m_entity, args...);
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
    World* const m_world;
    std::shared_ptr<std::vector<entt::entity>> m_despawns;
    std::shared_ptr<std::vector<entt::entity>> m_recursive_despawns;

   public:
    Command(World* world)
        : m_world(world),
          m_despawns(std::make_shared<std::vector<entt::entity>>()),
          m_recursive_despawns(std::make_shared<std::vector<entt::entity>>()) {}

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
        auto m_registry = &m_world->m_registry;
        auto entity = m_registry->create();
        app_tools::registry_emplace(
            m_registry, entity, std::forward<Args>(args)...
        );
        return entity;
    }

    /**
     * @brief Get the entity command for the entity.
     *
     * @param entity The entity id.
     * @return `EntityCommand` The entity command.
     */
    EntityCommand entity(entt::entity entity) {
        return EntityCommand(
            m_world,
            entity,
            m_despawns,
            m_recursive_despawns
        );
    }

    /*! @brief Insert a resource.
     * If the resource already exists, nothing will happen.
     * @tparam T The type of the resource.
     * @tparam Args The types of the arguments to be passed to the
     * @param args The arguments to be passed to the constructor of T
     */
    template <typename T, typename... Args>
    void insert_resource(Args&&... args) {
        auto m_resources = &m_world->m_resources;
        if (m_resources->find(&typeid(T)) == m_resources->end()) {
            m_resources->emplace(std::make_pair(
                &typeid(T), std::static_pointer_cast<void>(
                                std::make_shared<std::remove_reference_t<T>>(
                                    std::forward<Args>(args)...
                                )
                            )
            ));
        }
    }

    template <typename T>
    void insert_resource(T&& res) {
        auto m_resources = &m_world->m_resources;
        if (m_resources->find(&typeid(T)) == m_resources->end()) {
            m_resources->emplace(std::make_pair(
                &typeid(T), std::static_pointer_cast<void>(
                                std::make_shared<std::remove_reference_t<T>>(
                                    std::forward<T>(res)
                                )
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
        auto m_resources = &m_world->m_resources;
        m_resources->erase(&typeid(T));
    }

    /*! @brief Insert Resource using default values.
     * If the resource already exists, nothing will happen.
     * @tparam T The type of the resource.
     */
    template <typename T>
    void init_resource() {
        auto m_resources = &m_world->m_resources;
        if (m_resources->find(&typeid(T)) == m_resources->end()) {
            auto res = std::static_pointer_cast<void>(
                std::make_shared<std::remove_reference_t<T>>()
            );
            m_resources->insert({&typeid(T), res});
        }
    }

    void end() {
        auto m_registry = &m_world->m_registry;
        for (auto entity : *m_recursive_despawns) {
            m_registry->destroy(entity);
        }
        m_recursive_despawns->clear();
        for (auto entity : *m_despawns) {
            m_registry->destroy(entity);
        }
        m_despawns->clear();
    }
};
}  // namespace app
}  // namespace pixel_engine
