#pragma once

#include <freetype/freetype.h>
#include <ft2build.h>
#include <pixel_engine/app.h>
#include <pixel_engine/render_vk.h>

#include <string>

namespace pixel_engine {
namespace font {
namespace components {
using namespace prelude;
using namespace render_vk::components;

struct Font {
    bool antialias = true;
    int pixels = 16;
    FT_Face font_face;
};

struct Text {
    Font font;
    std::wstring text;
    float height = 0;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float center[2] = {0.0f, 0.0f};
};

struct TextRenderer {
    DescriptorSetLayout text_descriptor_set_layout;
    DescriptorPool text_descriptor_pool;
    DescriptorSet text_descriptor_set;

    RenderPass text_render_pass;
    Pipeline text_pipeline;

    Buffer text_vertex_buffer;

    Buffer text_texture_staging_buffer;
    Image text_texture_image;
    ImageView text_texture_image_view;
};
}  // namespace components
}  // namespace font
}  // namespace pixel_engine