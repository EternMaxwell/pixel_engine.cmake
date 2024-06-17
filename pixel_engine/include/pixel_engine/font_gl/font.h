#pragma once

#include "components.h"
#include "pixel_engine/entity.h"
#include "resources.h"
#include "systems.h"

namespace pixel_engine {
    namespace font_gl {
        using namespace components;
        using namespace systems;
        using namespace resources;
        using namespace prelude;

        class FontGLPlugin : public Plugin {
           public:
            void build(App& app) { app.add_system(Startup{}, insert_ft2_library); }
        };
    }  // namespace font_gl
}  // namespace pixel_engine