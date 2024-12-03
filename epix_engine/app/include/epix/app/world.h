#pragma once

#include <sparsepp/spp.h>

#include <entt/entity/registry.hpp>
#include <memory>
#include <typeindex>

#include "event_queue.h"

namespace epix::app {
struct World {
    entt::registry m_registry;
    spp::sparse_hash_map<std::type_index, std::shared_ptr<void>> m_resources;
    spp::sparse_hash_map<std::type_index, std::unique_ptr<EventQueueBase>>
        m_event_queues;
};
}  // namespace epix::app