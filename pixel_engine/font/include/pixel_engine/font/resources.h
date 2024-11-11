#pragma once

#include <freetype/freetype.h>
#include <ft2build.h>
#include <pixel_engine/app.h>
#include <pixel_engine/render_vk.h>

#include <filesystem>

#include "components.h"

namespace pixel_engine {
namespace font {
namespace resources {
using namespace prelude;
using namespace render_vk::components;
using namespace font::components;
namespace tools {
struct Glyph {
    struct ivec2 {
        int32_t x;
        int32_t y;
    };
    struct uvec2 {
        uint32_t x;
        uint32_t y;
    };
    struct vec2 {
        float x;
        float y;
    };
    uvec2 size;
    ivec2 bearing;
    ivec2 advance;
    int image_index;
    vec2 uv_1;
    vec2 uv_2;
};
struct GlyphMap {
    const Glyph& get_glyph(uint32_t index) const;
    void add_glyph(uint32_t index, const Glyph& glyph);
    bool contains(uint32_t index) const;

   private:
    std::shared_ptr<std::unordered_map<uint32_t, Glyph>> glyphs =
        std::make_shared<std::unordered_map<uint32_t, Glyph>>();
};
}  // namespace tools
using namespace tools;
namespace res_ogl {
struct FT2LibGL {
    using texture_handle = uint64_t;
    using texture_id     = uint32_t;
    using buffer_id      = uint32_t;
    struct CharLoadingState {
        uint32_t current_x           = 0;
        uint32_t current_y           = 0;
        uint32_t current_layer       = 0;
        uint32_t current_line_height = 0;
    };
    FT_Face load_font(const std::string& file_path);
    void init();
    void destroy();
    const uint32_t font_texture_width  = 2048;
    const uint32_t font_texture_height = 2048;
    const uint32_t font_texture_layers = 256;
    buffer_id font_texture_buffer;
    spp::sparse_hash_map<std::string, FT_Face> font_faces;
    spp::sparse_hash_map<Font, texture_handle> font_texture_handles;
    spp::sparse_hash_map<Font, texture_id> font_texture_ids;
    spp::sparse_hash_map<Font, CharLoadingState> char_loading_states;
};
}  // namespace res_ogl
namespace vulkan {
struct FT2Library {
    struct CharLoadingState {
        uint32_t current_x           = 0;
        uint32_t current_y           = 0;
        uint32_t current_layer       = 0;
        uint32_t current_line_height = 0;
    };
    FT_Face load_font(const std::string& file_path);
    std::tuple<Image, ImageView, GlyphMap>& get_font_texture(
        const Font& font,
        Device& device,
        CommandPool& command_pool,
        Queue& queue
    );
    uint32_t font_index(const Font& font);
    void add_char_to_texture(
        const Font& font,
        wchar_t c,
        Device& device,
        CommandPool& command_pool,
        Queue& queue
    );
    void add_chars_to_texture(
        const Font& font,
        const std::wstring& text,
        Device& device,
        CommandPool& command_pool,
        Queue& queue
    );
    std::optional<const Glyph> get_glyph(const Font& font, wchar_t c);
    std::optional<const Glyph> get_glyph_add(
        const Font& font,
        wchar_t c,
        Device& device,
        CommandPool& command_pool,
        Queue& queue
    );

    void clear_font_textures(Device& device);
    void destroy();

    const uint32_t font_texture_width  = 2048;
    const uint32_t font_texture_height = 2048;
    const uint32_t font_texture_layers = 256;
    FT_Library library;
    std::unordered_map<std::string, FT_Face> font_faces;
    std::unordered_map<Font, uint32_t> font_texture_index;
    std::unordered_map<Font, std::tuple<Image, ImageView, GlyphMap>>
        font_textures;
    std::unordered_map<Font, CharLoadingState> char_loading_states;
    DescriptorSetLayout font_texture_descriptor_set_layout;
    DescriptorPool font_texture_descriptor_pool;
    DescriptorSet font_texture_descriptor_set;
    uint32_t font_texture_count = 0;
};
}  // namespace vulkan
}  // namespace resources
}  // namespace font
}  // namespace pixel_engine