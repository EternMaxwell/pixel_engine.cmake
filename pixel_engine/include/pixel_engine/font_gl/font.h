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

        class FontGLPlugin : public Plugin {
           public:
            SystemNode library_insert_node;

            void build(App& app) {
                app.add_system(Startup{}, insert_ft2_library, &library_insert_node)
                    .add_system_main(
                        Startup{}, create_pipeline,
                        after(app.get_plugin<render_gl::RenderGLPlugin>()->context_creation_node))
                    .add_system_main(Render{}, draw);
            }
        };
    }  // namespace font_gl
}  // namespace pixel_engine