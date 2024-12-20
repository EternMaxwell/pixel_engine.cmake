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
struct Ivec2Hash {
    std::size_t operator()(const glm::ivec2& vec) const {
        return std::hash<size_t>()(*(size_t*)&vec);
    }
};
template <typename T>
struct SimulationCollisions {
    struct ChunkCollisions {
        std::vector<std::vector<std::vector<glm::ivec2>>> collisions = {};
        std::vector<T> user_data                                     = {};
        operator bool() const { return !collisions.empty(); }
        bool operator!() const { return collisions.empty(); }
    };
    using Grid =
        epix::utils::grid2d::ExtendableGrid2D<std::optional<ChunkCollisions>>;
    Grid collisions;
    spp::sparse_hash_set<glm::ivec2, Ivec2Hash> modified;
    std::unique_ptr<BS::thread_pool> thread_pool;

    SimulationCollisions() : thread_pool(std::make_unique<BS::thread_pool>()) {}
    void sync(const epix::world::sand::components::Simulation& sim) {
        modified.clear();
        for (auto [pos, chunk] : sim.chunk_map()) {
            collisions.try_emplace(
                pos.x, pos.y, SimulationCollisions::ChunkCollisions{}
            );
        }
        for (auto [pos, chunk] : sim.chunk_map()) {
            if (!collisions.contains(pos.x, pos.y)) continue;
            if (!chunk.should_update()) continue;
            modified.insert(pos);
            thread_pool->submit_task([this, &sim, &chunk, pos]() {
                auto collisions = get_chunk_collision(sim, chunk);
                this->collisions.get(pos.x, pos.y)->collisions = collisions;
            });
        }
        thread_pool->wait();
    }
};
template <>
struct SimulationCollisions<void> {
    struct ChunkCollisions {
        std::vector<std::vector<std::vector<glm::ivec2>>> collisions = {};
        operator bool() const { return !collisions.empty(); }
        bool operator!() const { return collisions.empty(); }
    };
    using Grid =
        epix::utils::grid2d::ExtendableGrid2D<std::optional<ChunkCollisions>>;
    Grid collisions;
    spp::sparse_hash_set<glm::ivec2, Ivec2Hash> modified;
    std::unique_ptr<BS::thread_pool> thread_pool;

    EPIX_API SimulationCollisions();
    EPIX_API void sync(const epix::world::sand::components::Simulation& sim);
};
}  // namespace epix::world::sand_physics
