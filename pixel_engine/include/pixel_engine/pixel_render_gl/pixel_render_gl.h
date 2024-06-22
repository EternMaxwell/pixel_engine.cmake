#pragma once

#include "components.h"
#include "systems.h"

namespace pixel_engine {
    namespace pixel_render_gl {
        using namespace components;
        using namespace systems;
        using namespace prelude;

        struct PixelRenderGLPlugin : public Plugin {
           public:
            void build(App& app) override {
                app.add_system_main(
                       Startup{}, create_pipeline, in_set(render_gl::RenderGLStartupSets::after_context_creation))
                    .add_system_main(Render{}, draw);
            }
        };
    }  // namespace pixel_render_gl
}  // namespace pixel_engine