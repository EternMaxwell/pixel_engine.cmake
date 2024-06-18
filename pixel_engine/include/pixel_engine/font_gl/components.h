#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>

#include "pixel_engine/components/components.h"
#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace font_gl {
        namespace components {
            using namespace prelude;
            using namespace core_components;
            /*! @brief Text component. Will be rendered if has Transform component.
             *  @brief text : Text to render, in unicode encoding.
             *  @brief size : Size to draw of the text.
             *  @brief pixels : Size in pixel of the text.
             *  @brief color : Color of the text.
             *  @brief center : Center of the text.
             *  @brief font_face : Font face, should be loaded through asset server.
             */
            struct Text {
                /*! @brief Text to render, in unicode encoding. */
                std::u8string text;
                /*! @brief Size to draw of the text. */
                float size = 0.1f;
                /*! @brief Size in pixel of the text. */
                int pixels = 16;
                /*! @brief Color of the text. */
                float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                float center[2] = {0.0f, 0.0f};
                /*! @brief Font face, should be loaded through asset server.*/
                FT_Face font_face;
            };

            struct TextBundle {
                Bundle bundle;
                Text text;
                Transform transform;
            };

            struct TextVertex {
                float position[3];
                float color[4];
                float tex_coords[2];
            };

            struct TextPipeline {};
        }  // namespace components
    }      // namespace font_gl
}  // namespace pixel_engine