#pragma once

#include <freetype/freetype.h>
#include <ft2build.h>


namespace pixel_engine {
namespace font_gl {
namespace resources {
struct FT2Library {
    FT_Library library;
};
}  // namespace resources
}  // namespace font_gl
}  // namespace pixel_engine