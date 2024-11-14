#include <glm/gtc/matrix_transform.hpp>

#include "pixel_engine/render/debug.h"

namespace pixel_engine::render::debug::vulkan::components {
void LineDrawer::begin(Device& device, Queue& queue, Swapchain& swapchain) {
    context = Context{device, queue};
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
    auto& extent   = swapchain.extent;
    glm::mat4 proj = glm::ortho(
        -(float)extent.width / 2.0f, (float)extent.width / 2.0f,
        (float)extent.height / 2.0f, -(float)extent.height / 2.0f, -1.0f, 1.0f
    );
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4* ubo = static_cast<glm::mat4*>(uniform_buffer.map(device));
    ubo[0]         = view;
    ubo[1]         = proj;
    uniform_buffer.unmap(device);
    context->mapped_model_buffer  = (glm::mat4*)model_buffer.map(device);
    context->mapped_vertex_buffer = (DebugVertex*)vertex_buffer.map(device);
    context->extent               = swapchain.extent;
}
void LineDrawer::flush() {
    if (!context.has_value()) return;
    if (context->vertex_count == 0 || context->model_count == 0) return;
    auto& ctx    = context.value();
    auto& device = *ctx.device;
    auto& queue  = *ctx.queue;
    command_buffer->begin(vk::CommandBufferBeginInfo{});
    vk::ClearValue clear_color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    vk::RenderPassBeginInfo render_pass_info;
    render_pass_info.setRenderPass(*render_pass);
    render_pass_info.setFramebuffer(*framebuffer);
    render_pass_info.setRenderArea({{0, 0}, ctx.extent});
    render_pass_info.setClearValues(clear_color);
    command_buffer->beginRenderPass(
        render_pass_info, vk::SubpassContents::eInline
    );
    command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    command_buffer->bindVertexBuffers(0, *vertex_buffer, {0});
    command_buffer->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0, *descriptor_set,
        {}
    );
    command_buffer->setScissor(0, vk::Rect2D({0, 0}, ctx.extent));
    command_buffer->setViewport(
        0, vk::Viewport(
               0.0f, 0.0f, (float)ctx.extent.width, (float)ctx.extent.height,
               0.0f, 1.0f
           )
    );
    command_buffer->draw(ctx.vertex_count, 1, 0, 0);
    command_buffer->endRenderPass();
    command_buffer->end();
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(*command_buffer);
    device->resetFences(*fence);
    queue->submit(submit_info, *fence);
    ctx.vertex_count = 0;
    ctx.model_count  = 0;
}
void LineDrawer::end() {
    if (!context.has_value()) return;
    flush();
    auto& ctx = context.value();
    model_buffer.unmap(*ctx.device);
    vertex_buffer.unmap(*ctx.device);
    context.reset();
}
void LineDrawer::setModel(const glm::mat4& model) {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.model_count >= 128) flush();
    ctx.mapped_model_buffer[ctx.model_count] = model;
    ctx.model_count++;
}
void LineDrawer::reset_cmd() {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    (*ctx.device)->waitForFences(*fence, VK_TRUE, UINT64_MAX);
    command_buffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
}
void LineDrawer::drawLine(
    const glm::vec3& start, const glm::vec3& end, const glm::vec4& color
) {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.model_count == 0) setModel(glm::mat4(1.0f));
    if (ctx.vertex_count + 2 >= 2048) flush();
    ctx.mapped_vertex_buffer[ctx.vertex_count] = {
        start, color, ctx.model_count - 1
    };
    ctx.vertex_count++;
    ctx.mapped_vertex_buffer[ctx.vertex_count] = {
        end, color, ctx.model_count - 1
    };
    ctx.vertex_count++;
}
void PointDrawer::begin(Device& device, Queue& queue, Swapchain& swapchain) {
    context = Context{device, queue};
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
    auto& extent   = swapchain.extent;
    glm::mat4 proj = glm::ortho(
        -(float)extent.width / 2.0f, (float)extent.width / 2.0f,
        (float)extent.height / 2.0f, -(float)extent.height / 2.0f, -1.0f, 1.0f
    );
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4* ubo = static_cast<glm::mat4*>(uniform_buffer.map(device));
    ubo[0]         = view;
    ubo[1]         = proj;
    uniform_buffer.unmap(device);
    context->mapped_model_buffer  = (glm::mat4*)model_buffer.map(device);
    context->mapped_vertex_buffer = (DebugVertex*)vertex_buffer.map(device);
    context->extent               = swapchain.extent;
}
void PointDrawer::flush() {
    if (!context.has_value()) return;
    if (context->vertex_count == 0 || context->model_count == 0) return;
    auto& ctx    = context.value();
    auto& device = *ctx.device;
    auto& queue  = *ctx.queue;
    command_buffer->begin(vk::CommandBufferBeginInfo{});
    vk::ClearValue clear_color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    vk::RenderPassBeginInfo render_pass_info;
    render_pass_info.setRenderPass(*render_pass);
    render_pass_info.setFramebuffer(*framebuffer);
    render_pass_info.setRenderArea({{0, 0}, ctx.extent});
    render_pass_info.setClearValues(clear_color);
    command_buffer->beginRenderPass(
        render_pass_info, vk::SubpassContents::eInline
    );
    command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    command_buffer->bindVertexBuffers(0, *vertex_buffer, {0});
    command_buffer->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0, *descriptor_set,
        {}
    );
    command_buffer->setScissor(0, vk::Rect2D({0, 0}, ctx.extent));
    command_buffer->setViewport(
        0, vk::Viewport(
               0.0f, 0.0f, (float)ctx.extent.width, (float)ctx.extent.height,
               0.0f, 1.0f
           )
    );
    command_buffer->draw(ctx.vertex_count, 1, 0, 0);
    command_buffer->endRenderPass();
    command_buffer->end();
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(*command_buffer);
    device->resetFences(*fence);
    queue->submit(submit_info, *fence);
    ctx.vertex_count = 0;
    ctx.model_count  = 0;
}
void PointDrawer::end() {
    if (!context.has_value()) return;
    flush();
    auto& ctx = context.value();
    model_buffer.unmap(*ctx.device);
    vertex_buffer.unmap(*ctx.device);
    context.reset();
}
void PointDrawer::setModel(const glm::mat4& model) {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.model_count >= 128) flush();
    ctx.mapped_model_buffer[ctx.model_count] = model;
    ctx.model_count++;
}
void PointDrawer::reset_cmd() {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    (*ctx.device)->waitForFences(*fence, VK_TRUE, UINT64_MAX);
    command_buffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
}
void PointDrawer::drawPoint(
    const glm::vec3& pos, const glm::vec4& color
) {
    if (!context.has_value()) return;
    auto& ctx = context.value();
    if (ctx.model_count == 0) setModel(glm::mat4(1.0f));
    if (ctx.vertex_count + 1 >= 2048) flush();
    ctx.mapped_vertex_buffer[ctx.vertex_count] = {pos, color, ctx.model_count - 1};
    ctx.vertex_count++;
}
}  // namespace pixel_engine::render::debug::vulkan::components