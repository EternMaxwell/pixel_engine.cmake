#include <unordered_map>

#include "pixel_engine/font.h"
#include "shaders/fragment_shader.h"
#include "shaders/vertex_shader.h"

using namespace pixel_engine;
using namespace pixel_engine::font;
using namespace pixel_engine::font::components;
using namespace pixel_engine::font::resources;
using namespace pixel_engine::font::systems;

static std::shared_ptr<spdlog::logger> logger =
    spdlog::default_logger()->clone("font");

void systems::insert_ft2_library(Command command) {
    FT2Library ft2_library;
    if (FT_Init_FreeType(&ft2_library.library)) {
        spdlog::error("Failed to initialize FreeType library");
    }
    command.insert_resource(ft2_library);
}

void systems::create_renderer(
    Command command,
    Query<Get<Device, Queue, CommandPool>, With<RenderContext>> query,
    Resource<FontPlugin> font_plugin
) {
    if (!query.single().has_value()) return;
    auto [device, queue, command_pool] = query.single().value();
    auto canvas_width                  = font_plugin->canvas_width;
    auto canvas_height                 = font_plugin->canvas_height;
    TextRenderer text_renderer;
    text_renderer.text_descriptor_set_layout = DescriptorSetLayout::create(
        device,
        {vk::DescriptorSetLayoutBinding()
             .setBinding(1)
             .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
             .setDescriptorCount(1)
             .setStageFlags(vk::ShaderStageFlagBits::eFragment),
         vk::DescriptorSetLayoutBinding()
             .setBinding(0)
             .setDescriptorType(vk::DescriptorType::eUniformBuffer)
             .setDescriptorCount(1)
             .setStageFlags(vk::ShaderStageFlagBits::eVertex)}
    );
    auto pool_sizes = std::vector<vk::DescriptorPoolSize>{
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
    };
    text_renderer.text_descriptor_pool = DescriptorPool::create(
        device, vk::DescriptorPoolCreateInfo()
                    .setMaxSets(1)
                    .setPoolSizes(pool_sizes)
                    .setFlags(
                        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet |
                        vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT
                    )
    );
    text_renderer.text_descriptor_set = DescriptorSet::create(
        device, vk::DescriptorSetAllocateInfo()
                    .setDescriptorPool(*text_renderer.text_descriptor_pool)
                    .setSetLayouts({*text_renderer.text_descriptor_set_layout})
    );
    text_renderer.text_uniform_buffer = Buffer::create_device(
        device, sizeof(TextUniformBuffer),
        vk::BufferUsageFlagBits::eUniformBuffer |
            vk::BufferUsageFlagBits::eTransferDst
    );
    vk::BufferCreateInfo buffer_create_info;
    buffer_create_info.setSize(sizeof(TextVertex) * 6 * 4096);
    buffer_create_info.setUsage(
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eVertexBuffer
    );
    buffer_create_info.setSharingMode(vk::SharingMode::eExclusive);
    AllocationCreateInfo alloc_info;
    alloc_info.setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    alloc_info.setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
    text_renderer.text_vertex_buffer =
        Buffer::create(device, buffer_create_info, alloc_info);

    vk::RenderPassCreateInfo render_pass_info;
    vk::AttachmentDescription color_attachment;
    color_attachment.setFormat(vk::Format::eB8G8R8A8Srgb);
    color_attachment.setSamples(vk::SampleCountFlagBits::e1);
    color_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
    color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal);
    color_attachment.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference color_attachment_ref;
    color_attachment_ref.setAttachment(0);
    color_attachment_ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachments(color_attachment_ref);
    render_pass_info.setAttachments({color_attachment});
    render_pass_info.setSubpasses({subpass});
    text_renderer.text_render_pass =
        RenderPass::create(device, render_pass_info);

    vk::GraphicsPipelineCreateInfo pipeline_info;

    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.setSetLayouts(
        {*text_renderer.text_descriptor_set_layout}
    );
    vk::PushConstantRange push_constant_range;
    push_constant_range.setSize(sizeof(glm::mat4));
    push_constant_range.setOffset(0);
    push_constant_range.setStageFlags(vk::ShaderStageFlagBits::eVertex);
    pipeline_layout_info.setPushConstantRanges(push_constant_range);
    text_renderer.text_pipeline_layout =
        PipelineLayout::create(device, pipeline_layout_info);

    pipeline_info.setLayout(*text_renderer.text_pipeline_layout);

    vk::ShaderModuleCreateInfo vert_shader_module_info;
    vert_shader_module_info.setCode(font_vk_vertex_spv);
    vk::ShaderModuleCreateInfo frag_shader_module_info;
    frag_shader_module_info.setCode(font_vk_fragment_spv);
    ShaderModule vert_shader_module =
        ShaderModule::create(device, vert_shader_module_info);
    ShaderModule frag_shader_module =
        ShaderModule::create(device, frag_shader_module_info);

    vk::PipelineShaderStageCreateInfo vert_shader_stage_info;
    vert_shader_stage_info.setStage(vk::ShaderStageFlagBits::eVertex);
    vert_shader_stage_info.setModule(vert_shader_module);
    vert_shader_stage_info.setPName("main");

    vk::PipelineShaderStageCreateInfo frag_shader_stage_info;
    frag_shader_stage_info.setStage(vk::ShaderStageFlagBits::eFragment);
    frag_shader_stage_info.setModule(frag_shader_module);
    frag_shader_stage_info.setPName("main");

    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        vert_shader_stage_info, frag_shader_stage_info
    };

    pipeline_info.setStages(shader_stages);

    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vk::VertexInputBindingDescription binding_description;
    binding_description.setBinding(0);
    binding_description.setStride(sizeof(TextVertex));
    binding_description.setInputRate(vk::VertexInputRate::eVertex);
    std::array<vk::VertexInputAttributeDescription, 4> attribute_descriptions =
        {vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(0)
             .setFormat(vk::Format::eR32G32B32Sfloat)
             .setOffset(offsetof(TextVertex, pos)),
         vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(1)
             .setFormat(vk::Format::eR32G32Sfloat)
             .setOffset(offsetof(TextVertex, uv)),
         vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(2)
             .setFormat(vk::Format::eR32G32B32A32Sfloat)
             .setOffset(offsetof(TextVertex, color)),
         vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(3)
             .setFormat(vk::Format::eR32Sint)
             .setOffset(offsetof(TextVertex, image_index))};
    vertex_input_info.setVertexBindingDescriptions({binding_description});
    vertex_input_info.setVertexAttributeDescriptions(attribute_descriptions);

    pipeline_info.setPVertexInputState(&vertex_input_info);

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.setTopology(vk::PrimitiveTopology::eTriangleList);
    input_assembly_info.setPrimitiveRestartEnable(VK_FALSE);

    pipeline_info.setPInputAssemblyState(&input_assembly_info);

    vk::DynamicState dynamic_states[] = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.setDynamicStates(dynamic_states);
    pipeline_info.setPDynamicState(&dynamic_state_info);

    vk::PipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.setViewportCount(1);
    viewport_state_info.setScissorCount(1);
    pipeline_info.setPViewportState(&viewport_state_info);

    vk::PipelineRasterizationStateCreateInfo rasterizer_info;
    rasterizer_info.setDepthClampEnable(VK_FALSE);
    rasterizer_info.setRasterizerDiscardEnable(VK_FALSE);
    rasterizer_info.setPolygonMode(vk::PolygonMode::eFill);
    rasterizer_info.setLineWidth(1.0f);
    rasterizer_info.setCullMode({});
    rasterizer_info.setFrontFace(vk::FrontFace::eClockwise);
    rasterizer_info.setDepthBiasEnable(VK_FALSE);
    pipeline_info.setPRasterizationState(&rasterizer_info);

    vk::PipelineMultisampleStateCreateInfo multisampling_info;
    multisampling_info.setSampleShadingEnable(VK_FALSE);
    multisampling_info.setRasterizationSamples(vk::SampleCountFlagBits::e1);
    pipeline_info.setPMultisampleState(&multisampling_info);

    vk::PipelineColorBlendAttachmentState color_blend_attachment;
    color_blend_attachment.setColorWriteMask(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );
    color_blend_attachment.setBlendEnable(VK_TRUE);
    color_blend_attachment.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
    color_blend_attachment.setDstColorBlendFactor(
        vk::BlendFactor::eOneMinusSrcAlpha
    );
    color_blend_attachment.setColorBlendOp(vk::BlendOp::eAdd);
    color_blend_attachment.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
    color_blend_attachment.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
    color_blend_attachment.setAlphaBlendOp(vk::BlendOp::eAdd);
    vk::PipelineColorBlendStateCreateInfo color_blending_info;
    color_blending_info.setLogicOpEnable(VK_FALSE);
    color_blending_info.setLogicOp(vk::LogicOp::eCopy);
    color_blending_info.setAttachmentCount(1);
    color_blending_info.setPAttachments(&color_blend_attachment);
    pipeline_info.setPColorBlendState(&color_blending_info);

    pipeline_info.setRenderPass(*text_renderer.text_render_pass);
    pipeline_info.setSubpass(0);

    text_renderer.text_pipeline = Pipeline::create(device, pipeline_info);

    vert_shader_module.destroy(device);
    frag_shader_module.destroy(device);

    text_renderer.text_texture_sampler = Sampler::create(
        device, vk::SamplerCreateInfo()
                    .setMagFilter(vk::Filter::eNearest)
                    .setMinFilter(vk::Filter::eNearest)
                    .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
                    .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
                    .setAddressModeW(vk::SamplerAddressMode::eClampToBorder)
                    .setAnisotropyEnable(VK_TRUE)
                    .setMaxAnisotropy(16)
                    .setBorderColor(vk::BorderColor::eIntTransparentBlack)
                    .setUnnormalizedCoordinates(VK_FALSE)
                    .setCompareEnable(VK_FALSE)
                    .setCompareOp(vk::CompareOp::eAlways)
                    .setMipmapMode(vk::SamplerMipmapMode::eNearest)
                    .setMipLodBias(0.0f)
                    .setMinLod(0.0f)
                    .setMaxLod(0.0f)
    );
    text_renderer.fence = Fence::create(
        device,
        vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled)
    );
    text_renderer.command_buffer = CommandBuffer::allocate_primary(
        device, command_pool
    );

    vk::DescriptorBufferInfo buffer_info;
    buffer_info.setBuffer(text_renderer.text_uniform_buffer);
    buffer_info.setOffset(0);
    buffer_info.setRange(sizeof(TextUniformBuffer));
    vk::WriteDescriptorSet descriptor_write;
    descriptor_write.setDstSet(*text_renderer.text_descriptor_set);
    descriptor_write.setDstBinding(0);
    descriptor_write.setDstArrayElement(0);
    descriptor_write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    descriptor_write.setDescriptorCount(1);
    descriptor_write.setPBufferInfo(&buffer_info);
    auto descriptor_writes = {descriptor_write};
    device->updateDescriptorSets(descriptor_writes, nullptr);

    command.spawn(text_renderer);
}

void systems::draw_text(
    Query<Get<TextRenderer>> text_renderer_query,
    Query<Get<Text, TextPos>> text_query,
    Resource<FT2Library> ft2_library,
    Query<Get<Device, Queue, CommandPool, Swapchain>, With<RenderContext>>
        swapchain_query,
    Resource<FontPlugin> font_plugin
) {
    if (!text_renderer_query.single().has_value()) return;
    if (!swapchain_query.single().has_value()) return;
    auto [device, queue, command_pool, swapchain] =
        swapchain_query.single().value();
    auto [text_renderer] = text_renderer_query.single().value();
    const auto& extent   = swapchain.extent;
    auto canvas_width    = font_plugin->canvas_width;
    auto canvas_height   = font_plugin->canvas_height;
    glm::mat4 inverse_y  = glm::scale(glm::mat4(1.0f), {1.0f, -1.0f, 1.0f});
    glm::mat4 proj =
        inverse_y *
        glm::ortho(0.0f, (float)extent.width, 0.0f, (float)extent.height);
    glm::mat4 view = glm::mat4(1.0f);
    TextUniformBuffer text_uniform_buffer;
    text_uniform_buffer.view           = view;
    text_uniform_buffer.proj           = proj;
    Buffer text_uniform_buffer_staging = Buffer::create_host(
        device, sizeof(TextUniformBuffer), vk::BufferUsageFlagBits::eTransferSrc
    );
    void* data = text_uniform_buffer_staging.map(device);
    memcpy(data, &text_uniform_buffer, sizeof(TextUniformBuffer));
    text_uniform_buffer_staging.unmap(device);
    auto cmd_uniform_copy =
        CommandBuffer::allocate_primary(device, command_pool);
    cmd_uniform_copy->begin(vk::CommandBufferBeginInfo{});
    vk::BufferCopy copy_region;
    copy_region.setSize(sizeof(TextUniformBuffer));
    cmd_uniform_copy->copyBuffer(
        text_uniform_buffer_staging, text_renderer.text_uniform_buffer,
        copy_region
    );
    vk::BufferMemoryBarrier barrier;
    barrier.setBuffer(text_renderer.text_uniform_buffer);
    barrier.setSize(sizeof(TextUniformBuffer));
    barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eUniformRead);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    cmd_uniform_copy->pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eVertexShader, {}, {}, barrier, {}
    );
    cmd_uniform_copy->end();
    auto submit_info = vk::SubmitInfo().setCommandBuffers(*cmd_uniform_copy);
    queue->submit(submit_info, nullptr);
    for (auto [text, pos] : text_query.iter()) {
        uint32_t vertices_count = 0;
        TextVertex* vertices =
            (TextVertex*)text_renderer.text_vertex_buffer.map(device);
        float ax = pos.x;
        float ay = pos.y;
        for (auto c : text.text) {
            auto glyph_opt = ft2_library->get_glyph_add(
                text.font, c, device, command_pool, queue
            );
            if (!glyph_opt.has_value()) continue;
            auto glyph = glyph_opt.value();
            if (glyph.size.x == 0 || glyph.size.y == 0) continue;
            float w  = glyph.size.x;
            float h  = glyph.size.y;
            float x  = ax + glyph.bearing.x;
            float y  = ay - (h - glyph.bearing.y);
            float u  = glyph.uv_1.x;
            float v  = glyph.uv_1.y;
            float u2 = glyph.uv_2.x;
            float v2 = glyph.uv_2.y;

            vertices[0] = {
                {x, y, 0.0f},
                {u, v},
                {text.color[0], text.color[1], text.color[2], text.color[3]},
                glyph.image_index
            };
            vertices[1] = {
                {x + w, y, 0.0f},
                {u2, v},
                {text.color[0], text.color[1], text.color[2], text.color[3]},
                glyph.image_index
            };
            vertices[2] = {
                {x + w, y + h, 0.0f},
                {u2, v2},
                {text.color[0], text.color[1], text.color[2], text.color[3]},
                glyph.image_index
            };
            vertices[3] = {
                {x, y + h, 0.0f},
                {u, v2},
                {text.color[0], text.color[1], text.color[2], text.color[3]},
                glyph.image_index
            };
            vertices[4] = {
                {x, y, 0.0f},
                {u, v},
                {text.color[0], text.color[1], text.color[2], text.color[3]},
                glyph.image_index
            };
            vertices[5] = {
                {x + w, y + h, 0.0f},
                {u2, v2},
                {text.color[0], text.color[1], text.color[2], text.color[3]},
                glyph.image_index
            };

            vertices += 6;
            vertices_count += 6;
            ax += glyph.advance.x;
            ay += glyph.advance.y;
        }
        auto [texture_image, font_image_view, glyph_map] =
            ft2_library->get_font_texture(
                text.font, device, command_pool, queue
            );
        text_renderer.text_vertex_buffer.unmap(device);
        vk::DescriptorImageInfo image_info;
        image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        image_info.setImageView(font_image_view);
        image_info.setSampler(text_renderer.text_texture_sampler);
        vk::WriteDescriptorSet descriptor_write;
        descriptor_write.setDstSet(*text_renderer.text_descriptor_set);
        descriptor_write.setDstBinding(1);
        descriptor_write.setDstArrayElement(0);
        descriptor_write.setDescriptorType(
            vk::DescriptorType::eCombinedImageSampler
        );
        descriptor_write.setDescriptorCount(1);
        descriptor_write.setPImageInfo(&image_info);
        auto descriptor_writes = {descriptor_write};
        device->updateDescriptorSets(descriptor_writes, nullptr);
        auto cmd_buffer = CommandBuffer::allocate_primary(device, command_pool);
        cmd_buffer->begin(vk::CommandBufferBeginInfo{});
        vk::RenderPassBeginInfo render_pass_info;
        render_pass_info.setRenderPass(*text_renderer.text_render_pass);
        auto image_view = swapchain.current_image_view();
        vk::FramebufferCreateInfo framebuffer_info;
        framebuffer_info.setRenderPass(*text_renderer.text_render_pass);
        framebuffer_info.setAttachments(*image_view);
        framebuffer_info.setWidth(extent.width);
        framebuffer_info.setHeight(extent.height);
        framebuffer_info.setLayers(1);
        Framebuffer framebuffer = Framebuffer::create(device, framebuffer_info);
        render_pass_info.setFramebuffer(*framebuffer);
        render_pass_info.setRenderArea({{0, 0}, extent});
        std::array<vk::ClearValue, 1> clear_values;
        clear_values[0].setColor(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
        render_pass_info.setClearValues(clear_values);
        cmd_buffer->beginRenderPass(
            render_pass_info, vk::SubpassContents::eInline
        );
        cmd_buffer->bindPipeline(
            vk::PipelineBindPoint::eGraphics, text_renderer.text_pipeline
        );
        cmd_buffer->bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            *text_renderer.text_pipeline_layout, 0,
            *text_renderer.text_descriptor_set, {}
        );
        cmd_buffer->bindVertexBuffers(
            0, *text_renderer.text_vertex_buffer, {0}
        );
        cmd_buffer->setViewport(
            0, vk::Viewport(
                   0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f,
                   1.0f
               )
        );
        cmd_buffer->setScissor(0, vk::Rect2D({0, 0}, extent));
        glm::mat4 model = glm::mat4(1.0f);
        cmd_buffer->pushConstants(
            *text_renderer.text_pipeline_layout,
            vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &model
        );
        cmd_buffer->draw(vertices_count, 1, 0, 0);
        cmd_buffer->endRenderPass();
        cmd_buffer->end();
        submit_info = vk::SubmitInfo().setCommandBuffers(*cmd_buffer);
        queue->submit(submit_info, nullptr);
        queue->waitIdle();
        cmd_buffer.free(device, command_pool);
        framebuffer.destroy(device);
    }
    queue->waitIdle();
    cmd_uniform_copy.free(device, command_pool);
    text_uniform_buffer_staging.destroy(device);
}

void systems::destroy_renderer(
    Query<Get<Device>, With<RenderContext>> query,
    Resource<FT2Library> ft2_library,
    Query<Get<TextRenderer>> text_renderer_query
) {
    if (!text_renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]        = query.single().value();
    auto [text_renderer] = text_renderer_query.single().value();
    logger->debug("destroy text renderer");
    ft2_library->clear_font_textures(device);
    text_renderer.text_render_pass.destroy(device);
    text_renderer.text_pipeline.destroy(device);
    text_renderer.text_descriptor_set.destroy(
        device, text_renderer.text_descriptor_pool
    );
    text_renderer.text_descriptor_pool.destroy(device);
    text_renderer.text_descriptor_set_layout.destroy(device);
    text_renderer.text_pipeline_layout.destroy(device);
    text_renderer.text_uniform_buffer.destroy(device);
    text_renderer.text_vertex_buffer.destroy(device);
    text_renderer.text_texture_sampler.destroy(device);
    text_renderer.fence.destroy(device);
    FT_Done_FreeType(ft2_library->library);
}