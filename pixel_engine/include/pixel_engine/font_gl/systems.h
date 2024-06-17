#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace font_gl {
        namespace systems {
            using namespace prelude;
            using namespace resources;

            void insert_ft2_library(Command command) {
                FT2Library ft2_library;
                FT_Error error = FT_Init_FreeType(&ft2_library.library);
                if (error) {
                    throw std::runtime_error("Failed to initialize FreeType library");
                }
                command.insert_resource(ft2_library);
            }
        }  // namespace systems
    }      // namespace font_gl
}  // namespace pixel_engine