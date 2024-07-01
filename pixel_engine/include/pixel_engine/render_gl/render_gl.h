#pragma once

#include "components.h"
#include "systems.h"

namespace pixel_engine {
    namespace render_gl {
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
            void build(App& app) override;
        };
    }  // namespace render_gl
}  // namespace pixel_engine
