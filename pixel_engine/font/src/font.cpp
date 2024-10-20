#include "pixel_engine/font.h"

using namespace pixel_engine;
using namespace pixel_engine::font;
using namespace pixel_engine::font::components;
using namespace pixel_engine::font::resources;
using namespace pixel_engine::font::systems;

static std::shared_ptr<spdlog::logger> logger =
    spdlog::default_logger()->clone("font");

using namespace pixel_engine::font::resources::tools;

const Glyph& GlyphMap::get_glyph(wchar_t c) const {
    if (glyphs->find(c) == glyphs->end()) {
        throw std::runtime_error("Glyph not found");
    }
    return glyphs->at(c);
}

void GlyphMap::add_glyph(wchar_t c, const Glyph& glyph) {
    glyphs->insert({c, glyph});
}

bool GlyphMap::contains(wchar_t c) const {
    return glyphs->find(c) != glyphs->end();
}

FT_Face FT2Library::load_font(const std::string& file_path) {
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

std::tuple<Image, ImageView, GlyphMap> FT2Library::get_font_texture(
    const Font& font, Device& device, CommandPool& command_pool, Queue& queue
) {
    if (font_textures.find(font) != font_textures.end()) {
        return font_textures[font];
    }

    Image image = Image::create(
        device,
        vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setFormat(vk::Format::eR8Uint)
            .setExtent(vk::Extent3D(font_texture_width, font_texture_height, 1))
            .setMipLevels(1)
            .setArrayLayers(font_texture_layers)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(
                vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst
            )
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined),
        AllocationCreateInfo()
            .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
    );

    auto cmd = CommandBuffer::allocate_primary(device, command_pool);
    cmd->begin(vk::CommandBufferBeginInfo{});
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(vk::ImageLayout::eUndefined);
    barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image);
    barrier.setSubresourceRange(vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, font_texture_layers
    ));
    barrier.setSrcAccessMask({});
    barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
    cmd->pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier
    );
    cmd->end();
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(*cmd);
    queue->submit(submit_info, nullptr);
    queue->waitIdle();
    cmd.free(device, command_pool);

    vk::ImageViewCreateInfo image_view_info;
    image_view_info.setImage(image);
    image_view_info.setViewType(vk::ImageViewType::e2DArray);
    image_view_info.setFormat(vk::Format::eR8Uint);
    image_view_info.setSubresourceRange(vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, font_texture_layers
    ));
    ImageView image_view = ImageView::create(device, image_view_info);

    GlyphMap glyph_map;

    font_textures[font] = {image, image_view, glyph_map};
    return {image, image_view, glyph_map};
}

void FT2Library::add_char_to_texture(
    const Font& font,
    wchar_t c,
    Device& device,
    CommandPool& command_pool,
    Queue& queue
) {
    auto [image, image_view, glyph_map] = font_textures[font];
    if (glyph_map.contains(c)) {
        return;
    }
    auto [current_x, current_y, current_layer, current_line_height] =
        char_loading_states[font];

    FT_Face face = font.font_face;
    if (FT_Set_Char_Size(face, 0, font.pixels, 1024, 1024)) {
        logger->error("Failed to set char size, skipping character");
        return;
    }
    if (FT_Set_Pixel_Sizes(face, 0, font.pixels)) {
        logger->error("Failed to set pixel size, skipping character");
        return;
    }
    int index = FT_Get_Char_Index(face, c);
    if (FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) {
        logger->error("Failed to load glyph, skipping character");
        return;
    }
    if (FT_Render_Glyph(
            face->glyph,
            font.antialias ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO
        )) {
        logger->error("Failed to render glyph, skipping character");
        return;
    }

    FT_GlyphSlot slot = face->glyph;
    FT_Bitmap bitmap = slot->bitmap;

    if (current_x + bitmap.width >= font_texture_width) {
        current_x = 0;
        current_y += current_line_height;
        current_line_height = 0;
    }

    if (current_y + bitmap.rows >= font_texture_height) {
        current_y = 0;
        current_layer += 1;
    }

    if (current_layer >= font_texture_layers) {
        logger->warn("Font texture is full, skipping character");
        return;
    }

    if (bitmap.rows > current_line_height) {
        current_line_height = bitmap.rows;
    }

    if (bitmap.width > 0 && bitmap.rows > 0) {
        logger->debug(
            "Adding character with bitmap size {},{} to font texture at {}, "
            "{}, layer {}",
            bitmap.width, bitmap.rows, current_x, current_y, current_layer
        );

        Buffer staging_buffer = Buffer::create_host(
            device, bitmap.width * bitmap.rows,
            vk::BufferUsageFlagBits::eTransferSrc
        );

        uint8_t* buffer = (uint8_t*)staging_buffer.map(device);
        for (int iy = 0; iy < bitmap.rows; iy++) {
            for (int ix = 0; ix < bitmap.width; ix++) {
                buffer[ix + (bitmap.rows - iy - 1) * bitmap.width] =
                    font.antialias
                        ? bitmap.buffer[ix + iy * bitmap.pitch]
                        : ((bitmap.buffer[ix / 8 + iy * bitmap.pitch] >>
                            (7 - ix % 8)) &
                           1) *
                              255;
            }
        }
        staging_buffer.unmap(device);

        auto command_buffer =
            CommandBuffer::allocate_primary(device, command_pool);
        command_buffer->begin(vk::CommandBufferBeginInfo{});
        vk::ImageMemoryBarrier barrier1;
        barrier1.setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier1.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier1.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier1.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier1.setImage(image);
        barrier1.setSubresourceRange(vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, current_layer, 1
        ));
        barrier1.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
        barrier1.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        command_buffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier1
        );
        vk::BufferImageCopy region;
        region.setBufferOffset(0);
        region.setBufferRowLength(bitmap.width);
        region.setBufferImageHeight(bitmap.rows);
        region.setImageSubresource(vk::ImageSubresourceLayers(
            vk::ImageAspectFlagBits::eColor, 0, current_layer, 1
        ));
        region.setImageOffset({(int)current_x, (int)current_y, 0});
        region.setImageExtent({bitmap.width, bitmap.rows, 1});
        command_buffer->copyBufferToImage(
            staging_buffer, image, vk::ImageLayout::eTransferDstOptimal, region
        );
        vk::ImageMemoryBarrier barrier;
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setImage(image);
        barrier.setSubresourceRange(vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, current_layer, 1
        ));
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        command_buffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier
        );
        command_buffer->end();
        vk::SubmitInfo submit_info;
        submit_info.setCommandBuffers(*command_buffer);
        queue->submit(submit_info, nullptr);
        queue->waitIdle();
        command_buffer.free(device, command_pool);
        staging_buffer.destroy(device);
    }

    Glyph glyph;
    glyph.advance = {
        static_cast<int>(slot->advance.x >> 6),
        static_cast<int>(slot->advance.y >> 6)
    };
    glyph.bearing = {
        static_cast<int>(slot->metrics.horiBearingX >> 6),
        static_cast<int>(slot->metrics.horiBearingY >> 6)
    };
    glyph.size = {bitmap.width, bitmap.rows};
    glyph.image_index = current_layer;
    glyph.uv_1 = {
        current_x / static_cast<float>(font_texture_width),
        current_y / static_cast<float>(font_texture_height)
    };
    glyph.uv_2 = {
        (current_x + bitmap.width) / static_cast<float>(font_texture_width),
        (current_y + bitmap.rows) / static_cast<float>(font_texture_height)
    };

    glyph_map.add_glyph(c, glyph);

    current_x += bitmap.width;
    char_loading_states[font] = {
        current_x, current_y, current_layer, current_line_height
    };
}

void FT2Library::add_chars_to_texture(
    const Font& font,
    const std::wstring& text,
    Device& device,
    CommandPool& command_pool,
    Queue& queue
) {
    for (auto c : text) {
        add_char_to_texture(font, c, device, command_pool, queue);
    }
}

std::optional<const Glyph> FT2Library::get_glyph(const Font& font, wchar_t c) {
    if (font_textures.find(font) == font_textures.end()) {
        return std::nullopt;
    }
    auto [image, image_view, glyph_map] = font_textures[font];
    if (!glyph_map.contains(c)) {
        return std::nullopt;
    }
    return glyph_map.get_glyph(c);
}

std::optional<const Glyph> FT2Library::get_glyph_add(
    const Font& font,
    wchar_t c,
    Device& device,
    CommandPool& command_pool,
    Queue& queue
) {
    if (font_textures.find(font) == font_textures.end()) {
        return std::nullopt;
    }
    auto [image, image_view, glyph_map] = font_textures[font];
    if (!glyph_map.contains(c)) {
        add_char_to_texture(font, c, device, command_pool, queue);
    }
    return get_glyph(font, c);
}

void FT2Library::clear_font_textures(Device& device) {
    for (auto& [font, texture] : font_textures) {
        auto [image, image_view, glyph_map] = texture;
        image.destroy(device);
        image_view.destroy(device);
    }
    font_textures.clear();
}

void FT2Library::destroy() {
    for (auto& [_, face] : font_faces) {
        FT_Done_Face(face);
    }
    FT_Done_FreeType(library);
}