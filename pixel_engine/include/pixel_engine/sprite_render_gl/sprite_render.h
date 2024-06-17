#pragma once

#include "components.h"
#include "pixel_engine/entity.h"
#include "systems.h"

namespace pixel_engine {
    namespace sprite_render_gl {
        using namespace components;
        using namespace systems;
        using namespace prelude;

        struct SpriteRenderGLPlugin : public Plugin {
           public:
            void build(App& app) override {
                app.add_system_main(
                       Startup{}, create_pipeline,
                       after(app.get_plugin<render_gl::RenderGLPlugin>()->context_creation_node))
                    .add_system_main(Render{}, draw);
            }
        };
    }  // namespace sprite_render_gl
}  // namespace pixel_engine