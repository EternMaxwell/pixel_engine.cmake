#pragma once

#include "pixel_engine/components/components.h"
#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace sprite_render_gl {
        namespace components {
            using namespace prelude;
            using namespace core_components;

            struct Sprite {
                int texture = 0;
                float center[2] = {0.5f, 0.5f};
                float size[2] = {1.0f, 1.0f};
                float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            };

            struct SpriteBundle {
                Bundle bundle;
                Sprite sprite;
                Transform transform;
            };
        }  // namespace components
    }      // namespace sprite_render_gl
}  // namespace pixel_engine