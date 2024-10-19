#pragma once

#include <filesystem>

#include "components.h"

namespace pixel_engine {
namespace font {
namespace resources {
struct FT2Library {
    FT_Library library;
    std::unordered_map<std::string, FT_Face> font_faces;

    FT_Face load_font(const std::string& file_path) {
        auto path = std::filesystem::absolute(file_path).string();
        if (font_faces.find(path) != font_faces.end()) {
            return font_faces[path];
        }
        FT_Face face;
        FT_Error error = FT_New_Face(library, path.c_str(), 0, &face);
        if (error) {
            throw std::runtime_error("Failed to load font: " + path);
        }
        font_faces[path] = face;
        return face;
    }
};
}  // namespace resources
}  // namespace font
}  // namespace pixel_engine