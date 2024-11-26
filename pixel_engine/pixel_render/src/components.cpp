#include <glm/gtc/matrix_transform.hpp>

#include "pixel_engine/render/pixel/components.h"

namespace pixel_engine::render::pixel {
namespace components {
PixelBlock PixelBlock::create(glm::uvec2 size) {
    PixelBlock block;
    block.size = size;
    block.pixels.resize(size.x * size.y, glm::vec4(0.0f));
    return block;
}
glm::vec4& PixelBlock::operator[](glm::uvec2 pos) {
    return pixels[pos.x + pos.y * size.x];
}
const glm::vec4& PixelBlock::operator[](glm::uvec2 pos) const {
    return pixels[pos.x + pos.y * size.x];
}

PixelBlockRenderer::Context::Context(Device& device, Queue& queue)
    : device(&device), queue(&queue) {}

void PixelBlockRenderer::begin(
    Device& device,
    Swapchain& swapchain,
    Queue& queue,
    const PixelUniformBuffer& pixel_uniform
) {
    if (context.has_value()) return;
    context    = Context{device, queue};
    auto& ctx  = context.value();
    ctx.extent = swapchain.extent;
    reset_cmd();
    framebuffer.destroy(device);
    framebuffer = Framebuffer::create(
        device, vk::FramebufferCreateInfo()
                    .setRenderPass(*render_pass)
                    .setAttachments(*swapchain.current_image_view())
                    .setWidth(swapchain.extent.width)
                    .setHeight(swapchain.extent.height)
                    .setLayers(1)
    );
    ctx.block_model_data =
        static_cast<PixelBlockData*>(block_model_buffer.map(device));
    ctx.vertex_data =
        static_cast<PixelBlockVertex*>(vertex_staging_buffer.map(device));
    *static_cast<PixelUniformBuffer*>(uniform_buffer.map(device)) =
        pixel_uniform;
    uniform_buffer.unmap(device);
}
void PixelBlockRenderer::begin(
    Device& device,
    Queue& queue,
    ImageView& render_target,
    vk::Extent2D extent,
    const PixelUniformBuffer& pixel_uniform
) {
    if (context.has_value()) return;
    context    = Context{device, queue};
    auto& ctx  = context.value();
    ctx.extent = extent;
    reset_cmd();
    framebuffer.destroy(device);
    framebuffer = Framebuffer::create(
        device, vk::FramebufferCreateInfo()
                    .setRenderPass(*render_pass)
                    .setAttachments(*render_target)
                    .setWidth(extent.width)
                    .setHeight(extent.height)
                    .setLayers(1)
    );
    ctx.block_model_data =
        static_cast<PixelBlockData*>(block_model_buffer.map(device));
    ctx.vertex_data =
        static_cast<PixelBlockVertex*>(vertex_staging_buffer.map(device));
    *static_cast<PixelUniformBuffer*>(uniform_buffer.map(device)) =
        pixel_uniform;
    uniform_buffer.unmap(device);
}

void PixelBlockRenderer::begin(
    Device& device,
    Queue& queue,
    Framebuffer& framebuffer,
    vk::Extent2D extent,
    const PixelUniformBuffer& pixel_uniform
) {
    if (context.has_value()) return;
    context    = Context{device, queue};
    auto& ctx  = context.value();
    ctx.extent = extent;
    reset_cmd();
    this->framebuffer = framebuffer;
    ctx.block_model_data =
        static_cast<PixelBlockData*>(block_model_buffer.map(device));
    ctx.vertex_data =
        static_cast<PixelBlockVertex*>(vertex_staging_buffer.map(device));
    *static_cast<PixelUniformBuffer*>(uniform_buffer.map(device)) =
        pixel_uniform;
    uniform_buffer.unmap(device);
}

void PixelBlockRenderer::end() {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    flush();
    vertex_staging_buffer.unmap(*ctx.device);
    block_model_buffer.unmap(*ctx.device);
    context.reset();
}

void PixelBlockRenderer::draw(
    const PixelBlock& block, const BlockPos2d& pos2d
) {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    set_block_data(block, pos2d);
    for (uint32_t y = 0; y < block.size.y; y++) {
        for (uint32_t x = 0; x < block.size.x; x++) {
            if (ctx.vertex_offset + 1 >= 4 * 1024 * 1024) {
                flush();
                reset_cmd();
                set_block_data(block, pos2d);
            }
            glm::vec4 color                      = block[{x, y}];
            ctx.vertex_data[ctx.vertex_offset++] = PixelBlockVertex{
                .color = color, .model_index = ctx.block_model_offset - 1
            };
        }
    }
}

void PixelBlockRenderer::reset_cmd() {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    (*ctx.device)->waitForFences(*fence, VK_TRUE, UINT64_MAX);
    command_buffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
}

void PixelBlockRenderer::set_block_data(
    const PixelBlock& block, const BlockPos2d& pos2d
) {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.block_model_offset + 1 >= 4 * 1024) {
        flush();
        reset_cmd();
    }
    glm::vec3 pos   = pos2d.pos;
    glm::vec2 scale = pos2d.scale;
    float rotation  = pos2d.rotation;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model           = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    model           = glm::scale(model, glm::vec3(scale, 1.0f));
    ctx.block_model_data[ctx.block_model_offset++] = PixelBlockData{
        .model = model, .size = block.size, .offset = ctx.vertex_offset
    };
}

void PixelBlockRenderer::flush() {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.block_model_offset) {
        command_buffer->begin(vk::CommandBufferBeginInfo{});
        // vk::ImageMemoryBarrier barrier;
        // barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
        // barrier.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal);
        // barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        // barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        // barrier.setImage(ctx.swapchain->current_image());
        // barrier.setSubresourceRange(vk::ImageSubresourceRange(
        //     vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        // ));
        // barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        // barrier.setDstAccessMask(
        //     vk::AccessFlagBits::eMemoryRead |
        //     vk::AccessFlagBits::eMemoryWrite |
        //     vk::AccessFlagBits::eColorAttachmentWrite
        // );
        // command_buffer->pipelineBarrier(
        //     vk::PipelineStageFlagBits::eColorAttachmentOutput,
        //     vk::PipelineStageFlagBits::eColorAttachmentOutput |
        //         vk::PipelineStageFlagBits::eBottomOfPipe,
        //     {}, {}, {}, barrier
        // );
        // copy vertex data and index data
        vk::BufferCopy copy_region;
        copy_region.setSrcOffset(0);
        copy_region.setDstOffset(0);
        copy_region.setSize(ctx.vertex_offset * sizeof(PixelBlockVertex));
        command_buffer->copyBuffer(
            *vertex_staging_buffer, *vertex_buffer, copy_region
        );
        vk::BufferMemoryBarrier buffer_barrier;
        buffer_barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        buffer_barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        buffer_barrier.setBuffer(*vertex_buffer);
        buffer_barrier.setOffset(0);
        buffer_barrier.setSize(VK_WHOLE_SIZE);
        buffer_barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        buffer_barrier.setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead
        );
        command_buffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eVertexInput, {}, {}, buffer_barrier, {}
        );
        vk::RenderPassBeginInfo render_pass_info;
        render_pass_info.setRenderPass(*render_pass);
        render_pass_info.setFramebuffer(*framebuffer);
        render_pass_info.setRenderArea({{0, 0}, ctx.extent});
        vk::ClearValue clear_color;
        clear_color.setColor(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
        render_pass_info.setClearValues(clear_color);
        command_buffer->beginRenderPass(
            render_pass_info, vk::SubpassContents::eInline
        );
        command_buffer->bindPipeline(
            vk::PipelineBindPoint::eGraphics, *graphics_pipeline
        );
        command_buffer->bindVertexBuffers(0, *vertex_buffer, {0});
        command_buffer->bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0,
            *descriptor_set, {}
        );
        command_buffer->setScissor(0, vk::Rect2D({0, 0}, ctx.extent));
        command_buffer->setViewport(
            0, vk::Viewport(
                   0.0f, 0.0f, ctx.extent.width, ctx.extent.height, 0.0f, 1.0f
               )
        );
        command_buffer->draw(ctx.vertex_offset, 1, 0, 0);
        command_buffer->endRenderPass();
        command_buffer->end();
        vk::SubmitInfo submit_info;
        submit_info.setCommandBuffers(*command_buffer);
        (*ctx.device)->resetFences(*fence);
        (*ctx.queue)->submit(submit_info, *fence);
        ctx.block_model_offset = 0;
        ctx.vertex_offset      = 0;
    }
}

PixelRenderer::Context::Context(Device& device, Queue& queue)
    : device(&device), queue(&queue) {}

void PixelRenderer::begin(
    Device& device,
    Swapchain& swapchain,
    Queue& queue,
    const PixelUniformBuffer& pixel_uniform
) {
    if (context.has_value()) return;
    context    = Context{device, queue};
    auto& ctx  = context.value();
    ctx.extent = swapchain.extent;
    reset_cmd();
    framebuffer.destroy(device);
    framebuffer = Framebuffer::create(
        device, vk::FramebufferCreateInfo()
                    .setRenderPass(*render_pass)
                    .setAttachments(*swapchain.current_image_view())
                    .setWidth(swapchain.extent.width)
                    .setHeight(swapchain.extent.height)
                    .setLayers(1)
    );
    ctx.model_data = static_cast<glm::mat4*>(block_model_buffer.map(device));
    ctx.vertex_data =
        static_cast<PixelVertex*>(vertex_staging_buffer.map(device));
    *static_cast<PixelUniformBuffer*>(uniform_buffer.map(device)) =
        pixel_uniform;
    uniform_buffer.unmap(device);
}
void PixelRenderer::begin(
    Device& device,
    Queue& queue,
    ImageView& render_target,
    vk::Extent2D extent,
    const PixelUniformBuffer& pixel_uniform
) {
    if (context.has_value()) return;
    context    = Context{device, queue};
    auto& ctx  = context.value();
    ctx.extent = extent;
    reset_cmd();
    framebuffer.destroy(device);
    framebuffer = Framebuffer::create(
        device, vk::FramebufferCreateInfo()
                    .setRenderPass(*render_pass)
                    .setAttachments(*render_target)
                    .setWidth(extent.width)
                    .setHeight(extent.height)
                    .setLayers(1)
    );
    ctx.model_data = static_cast<glm::mat4*>(block_model_buffer.map(device));
    ctx.vertex_data =
        static_cast<PixelVertex*>(vertex_staging_buffer.map(device));
    *static_cast<PixelUniformBuffer*>(uniform_buffer.map(device)) =
        pixel_uniform;
    uniform_buffer.unmap(device);
}

void PixelRenderer::begin(
    Device& device,
    Queue& queue,
    Framebuffer& framebuffer,
    vk::Extent2D extent,
    const PixelUniformBuffer& pixel_uniform
) {
    if (context.has_value()) return;
    context    = Context{device, queue};
    auto& ctx  = context.value();
    ctx.extent = extent;
    reset_cmd();
    this->framebuffer = framebuffer;
    ctx.model_data    = static_cast<glm::mat4*>(block_model_buffer.map(device));
    ctx.vertex_data =
        static_cast<PixelVertex*>(vertex_staging_buffer.map(device));
    *static_cast<PixelUniformBuffer*>(uniform_buffer.map(device)) =
        pixel_uniform;
    uniform_buffer.unmap(device);
}

void PixelRenderer::end() {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    flush();
    vertex_staging_buffer.unmap(*ctx.device);
    block_model_buffer.unmap(*ctx.device);
    context.reset();
}

void PixelRenderer::draw(const glm::vec4& color, const glm::vec2& pos) {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.vertex_offset + 1 >= 4 * 1024 * 1024) {
        flush();
        reset_cmd();
    }
    if (!ctx.model_offset) return;
    ctx.vertex_data[ctx.vertex_offset++] = PixelVertex{
        .color = color, .pos = pos, .model_index = ctx.model_offset - 1
    };
}

void PixelRenderer::reset_cmd() {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    (*ctx.device)->waitForFences(*fence, VK_TRUE, UINT64_MAX);
    command_buffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
}

void PixelRenderer::set_model(const glm::mat4& model) {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.model_offset + 1 >= 4 * 1024) {
        flush();
        reset_cmd();
    }
    ctx.model_data[ctx.model_offset++] = model;
}

void PixelRenderer::set_model(
    const glm::vec2& pos, const glm::vec2& scale, float rotation
) {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.model_offset + 1 >= 4 * 1024) {
        flush();
        reset_cmd();
    }
    glm::mat4 model = glm::translate(glm::mat4(1.0f), {pos.x, pos.y, 0.0f});
    model           = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    model           = glm::scale(model, {scale.x, scale.y, 1.0f});
    ctx.model_data[ctx.model_offset++] = model;
}

void PixelRenderer::flush() {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.model_offset) {
        command_buffer->begin(vk::CommandBufferBeginInfo{});
        // vk::ImageMemoryBarrier barrier;
        // barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
        // barrier.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal);
        // barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        // barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        // barrier.setImage(ctx.swapchain->current_image());
        // barrier.setSubresourceRange(vk::ImageSubresourceRange(
        //     vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        // ));
        // barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        // barrier.setDstAccessMask(
        //     vk::AccessFlagBits::eMemoryRead |
        //     vk::AccessFlagBits::eMemoryWrite |
        //     vk::AccessFlagBits::eColorAttachmentWrite
        // );
        // command_buffer->pipelineBarrier(
        //     vk::PipelineStageFlagBits::eColorAttachmentOutput,
        //     vk::PipelineStageFlagBits::eColorAttachmentOutput |
        //         vk::PipelineStageFlagBits::eBottomOfPipe,
        //     {}, {}, {}, barrier
        // );
        // copy vertex data and index data
        vk::BufferCopy copy_region;
        copy_region.setSrcOffset(0);
        copy_region.setDstOffset(0);
        copy_region.setSize(ctx.vertex_offset * sizeof(PixelVertex));
        command_buffer->copyBuffer(
            *vertex_staging_buffer, *vertex_buffer, copy_region
        );
        vk::BufferMemoryBarrier buffer_barrier;
        buffer_barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        buffer_barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        buffer_barrier.setBuffer(*vertex_buffer);
        buffer_barrier.setOffset(0);
        buffer_barrier.setSize(VK_WHOLE_SIZE);
        buffer_barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        buffer_barrier.setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead
        );
        command_buffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eVertexInput, {}, {}, buffer_barrier, {}
        );
        vk::RenderPassBeginInfo render_pass_info;
        render_pass_info.setRenderPass(*render_pass);
        render_pass_info.setFramebuffer(*framebuffer);
        render_pass_info.setRenderArea({{0, 0}, ctx.extent});
        vk::ClearValue clear_color;
        clear_color.setColor(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
        render_pass_info.setClearValues(clear_color);
        command_buffer->beginRenderPass(
            render_pass_info, vk::SubpassContents::eInline
        );
        command_buffer->bindPipeline(
            vk::PipelineBindPoint::eGraphics, *graphics_pipeline
        );
        command_buffer->bindVertexBuffers(0, *vertex_buffer, {0});
        command_buffer->bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0,
            *descriptor_set, {}
        );
        command_buffer->setScissor(0, vk::Rect2D({0, 0}, ctx.extent));
        command_buffer->setViewport(
            0, vk::Viewport(
                   0.0f, 0.0f, ctx.extent.width, ctx.extent.height, 0.0f, 1.0f
               )
        );
        command_buffer->draw(ctx.vertex_offset, 1, 0, 0);
        command_buffer->endRenderPass();
        command_buffer->end();
        vk::SubmitInfo submit_info;
        submit_info.setCommandBuffers(*command_buffer);
        (*ctx.device)->resetFences(*fence);
        (*ctx.queue)->submit(submit_info, *fence);
        ctx.model_offset  = 0;
        ctx.vertex_offset = 0;
    }
}
}  // namespace components
}  // namespace pixel_engine::render::pixel