#pragma once

#include <sparsepp/spp.h>

#include <entt/entity/registry.hpp>
#include <memory>

#include "event_queue.h"

namespace pixel_engine::app {
struct World {
    entt::registry m_registry;
    spp::sparse_hash_map<entt::entity, spp::sparse_hash_set<entt::entity>>
        m_entity_tree;
    spp::sparse_hash_map<const type_info*, std::unique_ptr<void>> m_resources;
    spp::sparse_hash_map<const type_info*, std::unique_ptr<EventQueueBase>>
        m_event_queues;
};
}  // namespace pixel_engine::app