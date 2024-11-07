#pragma once

#include <freetype/freetype.h>
#include <ft2build.h>
#include <pixel_engine/app.h>
#include <pixel_engine/render_ogl.h>
#include <pixel_engine/render_vk.h>

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace pixel_engine {
namespace font {
namespace components {
using namespace prelude;
using namespace render_vk::components;
struct Font {
    bool antialias = true;
    int pixels     = 64;
    FT_Face font_face;

    bool operator==(const Font& other) const {
        return font_face == other.font_face && pixels == other.pixels &&
               antialias == other.antialias;
    }
};

struct TextUniformBuffer {
    glm::mat4 view;
    glm::mat4 proj;
};

struct Text {
    Font font;
    std::wstring text;
    float height    = 0;
    float color[4]  = {1.0f, 1.0f, 1.0f, 1.0f};
    float center[2] = {0.0f, 0.0f};
};

struct TextPos {
    int x;
    int y;
};

struct TextVertex {
    float pos[3];
    float uv[2];
    float color[4];
    int image_index;
};

struct TextRenderer {
    DescriptorSetLayout text_descriptor_set_layout;
    DescriptorPool text_descriptor_pool;
    DescriptorSet text_descriptor_set;

    RenderPass text_render_pass;
    PipelineLayout text_pipeline_layout;
    Pipeline text_pipeline;

    Buffer text_uniform_buffer;

    Buffer text_vertex_buffer;

    Sampler text_texture_sampler;
};

struct TextRendererGl {
    uint32_t text_program;
    uint32_t text_vao;
    uint32_t text_vbo;
    uint32_t text_ibo;
    uint32_t text_ubo;
};
}  // namespace components
}  // namespace font
}  // namespace pixel_engine
template <>
struct std::hash<pixel_engine::font::components::Font> {
    size_t operator()(const pixel_engine::font::components::Font& font) const {
        return (std::hash<int>()(font.pixels) ^
                std::hash<bool>()(font.antialias)) *
               std::hash<FT_Face>()(font.font_face);
    }
};
