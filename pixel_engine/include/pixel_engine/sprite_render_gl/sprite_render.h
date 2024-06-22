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
            void build(App& app) override {
                app.add_system_main(
                       Startup{}, create_pipeline, in_set(render_gl::RenderGLStartupSets::after_context_creation))
                    .configure_sets(
                        SpriteRenderGLSets::before_draw, SpriteRenderGLSets::draw, SpriteRenderGLSets::after_draw)
                    .add_system_main(Render{}, draw, in_set(SpriteRenderGLSets::draw));
            }
        };
    }  // namespace sprite_render_gl
}  // namespace pixel_engine