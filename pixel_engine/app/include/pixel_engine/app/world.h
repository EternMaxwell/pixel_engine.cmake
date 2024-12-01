#pragma once

#include <sparsepp/spp.h>

#include <entt/entity/registry.hpp>
#include <memory>

#include "event_queue.h"
#include <typeindex>

namespace pixel_engine::app {
struct World {
    entt::registry m_registry;
    spp::sparse_hash_map<std::type_index, std::shared_ptr<void>> m_resources;
    spp::sparse_hash_map<std::type_index, std::unique_ptr<EventQueueBase>>
        m_event_queues;
};
}  // namespace pixel_engine::app