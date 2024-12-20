#include "epix/world/sand_physics.h"

struct ChunkConverter {
    const epix::world::sand::components::Simulation& sim;
    const epix::world::sand::components::Simulation::Chunk& chunk;

    bool contains(int x, int y) const {
        auto& cell = chunk.get(x, y);
        if (!cell) return false;
        auto& elem = sim.registry().get_elem(cell.elem_id);
        if (elem.is_gas() || elem.is_liquid()) return false;
        if (elem.is_powder() && cell.freefall) return false;
        return true;
    }
    glm::ivec2 size() const { return chunk.size(); }
};

namespace epix::world::sand_physics {
EPIX_API std::vector<std::vector<std::vector<glm::ivec2>>> get_chunk_collision(
    const epix::world::sand::components::Simulation& sim,
    const epix::world::sand::components::Simulation::Chunk& chunk
) {
    ChunkConverter grid{sim, chunk};
    return epix::utils::grid2d::get_polygon_multi(grid);
}
EPIX_API SimulationCollisions<void>::SimulationCollisions()
    : thread_pool(
          std::make_unique<BS::thread_pool>(std::thread::hardware_concurrency())
      ) {}
EPIX_API void SimulationCollisions<void>::sync(
    const epix::world::sand::components::Simulation& sim
) {
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
}  // namespace epix::world::sand_physics