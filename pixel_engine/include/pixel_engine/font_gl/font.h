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
            void build(App& app) {
                app.configure_sets(FontGLSets::insert_library, FontGLSets::after_insertion)
                    .add_system(PreStartup{}, insert_ft2_library, in_set(FontGLSets::insert_library))
                    .add_system_main(
                        PreStartup{}, create_pipeline, in_set(render_gl::RenderGLStartupSets::after_context_creation))
                    .add_system_main(Render{}, draw);
            }
        };
    }  // namespace font_gl
}  // namespace pixel_engine