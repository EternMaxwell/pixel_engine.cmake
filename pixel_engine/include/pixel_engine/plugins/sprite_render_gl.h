#pragma once

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace plugins {
        namespace sprite_render_gl {
            struct Sprite {
                int texture;
                float origin[2];
                float size[2];
            };
        }

        class SpriteRenderGLPlugin : public Plugin {
           public:
            void build(App& app) override {}
        };
    }  // namespace plugins
}  // namespace pixel_engine