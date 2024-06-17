#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

namespace pixel_engine {
    namespace font_gl {
        namespace resources {
            struct FT2Library {
                FT_Library library;
            };
        }
    }  // namespace font_gl
}  // namespace pixel_engine