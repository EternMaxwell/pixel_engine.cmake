#pragma once

#include <freetype/freetype.h>
#include <ft2build.h>
#include <glad/glad.h>

#include <fstream>

#include "pixel_engine/entity.h"
#include "pixel_engine/render_gl/render_gl.h"

namespace pixel_engine {
namespace asset_server_gl {
namespace resources {

using namespace pixel_engine::render_gl::components;
class AssetServerGL {
   private:
    std::string m_base_path;
    std::unordered_map<std::string, FT_Face> m_font_faces;

   public:
    EPIX_API AssetServerGL(const std::string& base_path = "./")
        : m_base_path(base_path) {}

    EPIX_API std::vector<char> load_shader_source(const std::string& path);
    EPIX_API ShaderPtr load_shader(const std::vector<char>& source, int type);
    EPIX_API ShaderPtr load_shader(const std::string& path, int type);
    EPIX_API TexturePtr load_image_2d(const std::string& path);
    EPIX_API SamplerPtr
    create_sampler(int wrap_s, int wrap_t, int min_filter, int mag_filter);
    EPIX_API FT_Face
    load_font(const FT_Library& library, const std::string& path);
};
}  // namespace resources
}  // namespace asset_server_gl
}  // namespace pixel_engine