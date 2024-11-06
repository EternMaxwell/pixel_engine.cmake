#pragma once

#include <sparsepp/spp.h>

#include <entt/entity/registry.hpp>
#include <memory>

#include "event_queue.h"

namespace pixel_engine::app {
struct World {
    entt::registry m_registry;
    spp::sparse_hash_map<const type_info*, std::shared_ptr<void>> m_resources;
    spp::sparse_hash_map<const type_info*, std::unique_ptr<EventQueueBase>>
        m_event_queues;
};
}  // namespace pixel_engine::app