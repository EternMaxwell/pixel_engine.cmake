#pragma once

#include "components.h"
#include "systems.h"

namespace pixel_engine {
    namespace render_gl {
        using namespace systems;
        using namespace prelude;

        enum class RenderGLStartupSets {
            context_creation,
            after_context_creation,
        };

        enum class RenderGLPreRenderSets {
            create_pipelines,
            after_create_pipelines,
        };

        class RenderGLPlugin : public entity::Plugin {
           public:
            void build(App& app) override {
                using namespace render_gl;
                using namespace window;
                app.configure_sets(RenderGLStartupSets::context_creation, RenderGLStartupSets::after_context_creation)
                    .add_system_main(
                        Startup{}, context_creation,
                        in_set(window::WindowStartUpSets::after_window_creation, RenderGLStartupSets::context_creation))
                    .configure_sets(
                        RenderGLPreRenderSets::create_pipelines, RenderGLPreRenderSets::after_create_pipelines)
                    .add_system_main(PreRender{}, create_pipelines, in_set(RenderGLPreRenderSets::create_pipelines))
                    .add_system_main(PreRender{}, clear_color)
                    .add_system_main(PreRender{}, update_viewport);
            }
        };
    }  // namespace render_gl
}  // namespace pixel_engine
