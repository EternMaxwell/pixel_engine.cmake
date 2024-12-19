#pragma once

#include <epix/physics2d.h>
#include <epix/world/sand.h>
#include <BS_thread_pool.hpp>

#include <glm/glm.hpp>

namespace epix::world::sand_physics {
EPIX_API std::vector<std::vector<std::vector<glm::ivec2>>> get_chunk_collision(
    const epix::world::sand::components::Simulation& sim,
    const epix::world::sand::components::Simulation::Chunk& chunk
);
struct SimulationCollisions {
    struct ChunkCollisions {
        std::vector<std::vector<std::vector<glm::ivec2>>> collisions = {};
        operator bool() const { return !collisions.empty(); }
        bool operator!() const { return collisions.empty(); }
    };
    using Grid =
        epix::utils::grid2d::ExtendableGrid2D<std::optional<ChunkCollisions>>;
    Grid collisions;
    std::unique_ptr<BS::thread_pool> thread_pool;

    EPIX_API SimulationCollisions();
    EPIX_API void sync(const epix::world::sand::components::Simulation& sim);
};
}  // namespace epix::world::sand_physics
