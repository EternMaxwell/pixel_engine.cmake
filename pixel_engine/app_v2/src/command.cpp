#include "pixel_engine/app/command.h"

using namespace pixel_engine::app;

EntityCommand::EntityCommand(
    World* world,
    entt::entity entity,
    std::shared_ptr<std::vector<entt::entity>> despawns,
    std::shared_ptr<std::vector<entt::entity>> recursive_despawns
)
    : m_registry(&world->m_registry),
      m_entity(entity),
      m_despawns(despawns),
      m_recursive_despawns(recursive_despawns) {}

void EntityCommand::despawn() { m_despawns->push_back(m_entity); }

void EntityCommand::despawn_recurse() {
    m_recursive_despawns->push_back(m_entity);
}

Command::Command(World* world)
    : m_world(world),
      m_despawns(std::make_shared<std::vector<entt::entity>>()),
      m_recursive_despawns(std::make_shared<std::vector<entt::entity>>()) {}

EntityCommand Command::entity(entt::entity entity) {
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