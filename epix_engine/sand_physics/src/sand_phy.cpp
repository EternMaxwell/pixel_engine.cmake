#include "epix/world/sand_physics.h"

struct ChunkConverter {
    const epix::world::sand::components::Simulation& sim;
    const epix::world::sand::components::Simulation::Chunk& chunk;
    const int offset_x = 0;
    const int offset_y = 0;
    const int width    = chunk.size().x;
    const int height   = chunk.size().y;

    bool contains(int x, int y) const {
        auto& cell = chunk.get(x, y);
        if (!cell) return false;
        auto& elem = sim.registry().get_elem(cell.elem_id);
        if (elem.is_gas() || elem.is_liquid()) return false;
        if (elem.is_powder() && cell.freefall) return false;
        return true;
    }
    glm::ivec2 size() const { return chunk.size(); }
    ChunkConverter sub(int x, int y, int w, int h) const {
        return ChunkConverter{sim, chunk, x, y, w, h};
    }
};

namespace epix::world::sand_physics {
std::vector<std::vector<std::vector<glm::ivec2>>> get_chunk_collision(
    const epix::world::sand::components::Simulation& sim,
    const epix::world::sand::components::Simulation::Chunk& chunk
) {
    ChunkConverter grid{sim, chunk};
    return epix::physics2d::utils::get_polygon_multi(grid);
}
}  // namespace epix::world::sand_physics