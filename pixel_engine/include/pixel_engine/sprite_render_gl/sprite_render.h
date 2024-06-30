#pragma once

#include "components.h"
#include "pixel_engine/entity.h"
#include "systems.h"

namespace pixel_engine {
    namespace sprite_render_gl {
        using namespace components;
        using namespace systems;
        using namespace prelude;

        enum class SpriteRenderGLSets {
            before_draw,
            draw,
            after_draw,
        };

        struct SpriteRenderGLPlugin : public Plugin {
           public:
            void build(App& app) override;
        };
    }  // namespace sprite_render_gl
}  // namespace pixel_engine