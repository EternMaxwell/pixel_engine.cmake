#pragma once

#include <ft2build.h>
#include <freetype/freetype.h>
#include <glad/glad.h>

#include <fstream>

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace asset_server_gl {
        namespace resources {
            class AssetServerGL {
               private:
                std::string m_base_path;
                std::unordered_map<std::string, FT_Face> m_font_faces;

               public:
                AssetServerGL(const std::string& base_path = "./") : m_base_path(base_path) {}

                std::vector<char> load_shader_source(const std::string& path);
                int loat_shader(const std::vector<char>& source, int type);
                int load_shader(const std::string& path, int type);
                int load_image_2d(const std::string& path);
                int create_sampler(int wrap_s, int wrap_t, int min_filter, int mag_filter);
                FT_Face load_font(const FT_Library& library, const std::string& path);
            };
        }  // namespace resources
    }      // namespace asset_server_gl
}  // namespace pixel_engine