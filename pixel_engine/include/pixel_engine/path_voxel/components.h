#pragma once

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace path_voxel {
        namespace components {
            struct Voxel {
                float color[3] = {1.0f, 1.0f, 1.0f};
            };

            struct Voxel_World {
                int width_x;
                int width_y;
                int width_z;
                std::vector<std::vector<std::vector<Voxel>>> voxels;
            };
        }  // namespace components
    }      // namespace path_voxel
}  // namespace pixel_engine