#include "pixel_engine/app/command.h"

using namespace pixel_engine::app;

EntityCommand::EntityCommand(
    World* world,
    Entity entity,
    std::shared_ptr<spp::sparse_hash_set<Entity>> despawns,
    std::shared_ptr<spp::sparse_hash_set<Entity>> recursive_despawns
)
    : m_registry(&world->m_registry),
      m_entity(entity),
      m_despawns(despawns),
      m_recursive_despawns(recursive_despawns) {}

void EntityCommand::despawn() { m_despawns->emplace(m_entity); }

void EntityCommand::despawn_recurse() {
    m_recursive_despawns->emplace(m_entity);
}

Command::Command(World* world)
    : m_world(world),
      m_despawns(std::make_shared<spp::sparse_hash_set<Entity>>()),
      m_recursive_despawns(std::make_shared<spp::sparse_hash_set<Entity>>()) {}

EntityCommand Command::entity(Entity entity) {
    return EntityCommand(m_world, entity, m_despawns, m_recursive_despawns);
}

void Command::end() {
    auto m_registry = &m_world->m_registry;
    for (auto entity : *m_recursive_despawns) {
        auto& children = m_registry->get_or_emplace<Children>(entity);
        for (auto child : children.children) {
            m_registry->destroy(child);
        }
        m_registry->destroy(entity);
    }
    m_recursive_despawns->clear();
    for (auto entity : *m_despawns) {
        m_registry->destroy(entity);
    }
    m_despawns->clear();
}