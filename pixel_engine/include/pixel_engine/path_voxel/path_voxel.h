#pragma once

#include "components.h"
#include "systems.h"

namespace pixel_engine {
    namespace path_voxel {
        using namespace prelude;

        struct PathVoxelPlugin : public Plugin {
           public:
            void build(App& app) override;
        };
    }  // namespace path_voxel
}  // namespace pixel_engine