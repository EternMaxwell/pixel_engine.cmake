#pragma once

#include "components.h"
#include "systems.h"

namespace pixel_engine {
    namespace render_gl {
        using namespace systems;
        using namespace prelude;

        class RenderGLPlugin : public entity::Plugin {
           public:
            SystemNode create_pipelines_node;
            SystemNode context_creation_node;

            void build(App& app) override {
                using namespace render_gl;
                using namespace window;
                app.add_system_main(
                       Startup{}, context_creation, &context_creation_node,
                       after(app.get_plugin<WindowPlugin>()->start_up_window_create_node))
                    .add_system_main(PreRender{}, create_pipelines, &create_pipelines_node)
                    .add_system_main(PreRender{}, clear_color);
            }
        };
    }  // namespace render_gl
}  // namespace pixel_engine
