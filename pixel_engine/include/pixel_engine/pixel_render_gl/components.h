#pragma once

#include "pixel_engine/components/components.h"
#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace pixel_render_gl {
        namespace components {
            using namespace prelude;
            using namespace core_components;

            /*! @brief A struct to store pixel data.
             *  @brief x: The x position of the pixel.
             *  @brief y: The y position of the pixel.
             *  @brief color: The color of the pixel.
             */
            struct Pixel {
                float position[3] = {0, 0, 0};
                float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            };

            /*! @brief A struct to store pixel datas. Uses vector to store pixels.
             *  @brief pixels: A vector of pixels.
             *  @brief To access functions of the vector, use the operator ->.
             */
            struct Pixels {
                std::vector<Pixel> pixels;
                auto& operator*() { return pixels; }
                auto operator->() const { return &pixels; }
            };

            struct PixelSize {
                float width = 1.0f;
                float height = 1.0f;
            };

            struct PixelGroupBundle {
                Bundle bundle;
                Pixels pixels;
                PixelSize size;
                Transform transform;
            };

            struct PixelPipeline {};
        }  // namespace components
    }      // namespace pixel_render_gl
}  // namespace pixel_engine