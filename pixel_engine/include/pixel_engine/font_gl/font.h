#pragma once

#include "components.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/render_gl/render_gl.h"
#include "resources.h"
#include "systems.h"

namespace pixel_engine {
    namespace font_gl {
        using namespace components;
        using namespace systems;
        using namespace resources;
        using namespace prelude;

        enum class FontGLSets {
            insert_library,
            after_insertion,
        };

        class FontGLPlugin : public Plugin {
           public:
            void build(App& app);
        };
    }  // namespace font_gl
}  // namespace pixel_engine