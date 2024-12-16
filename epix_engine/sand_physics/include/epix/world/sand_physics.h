#pragma once

#include <epix/physics2d.h>
#include <epix/world/sand.h>

#include <glm/glm.hpp>

namespace epix::world::sand_physics {
EPIX_API std::vector<std::vector<std::vector<glm::ivec2>>> get_chunk_collision(
    const epix::world::sand::components::Simulation& sim,
    const epix::world::sand::components::Simulation::Chunk& chunk
);
}
