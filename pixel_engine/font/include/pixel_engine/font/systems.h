#pragma once

#include <pixel_engine/render_vk.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include "components.h"
#include "resources.h"
#include "shaders/fragment_shader.h"
#include "shaders/vertex_shader.h"

namespace pixel_engine {
namespace font {
namespace systems {
using namespace prelude;
using namespace font::components;
using namespace render_vk::components;
using namespace font::resources;

static std::shared_ptr<spdlog::logger> logger =
    spdlog::default_logger()->clone("font");

void insert_ft2_library(Command command) {
    FT2Library ft2_library;
    if (FT_Init_FreeType(&ft2_library.library)) {
        spdlog::error("Failed to initialize FreeType library");
    }
    command.insert_resource(ft2_library);
}
void create_renderer(
    Command command,
    Query<Get<Device, Queue, CommandPool>, With<RenderContext>> query
) {
    logger->set_level(spdlog::level::debug);
    if (!query.single().has_value()) return;
    auto [device, queue, command_pool] = query.single().value();
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
        vk::BufferUsageFlagBits::eUniformBuffer
    );
    text_renderer.text_vertex_buffer = Buffer::create_device(
        device, 1024,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer
    );
    text_renderer.text_texture_staging_buffer = Buffer::create_host(
        device, 1024 * 1024 * 4, vk::BufferUsageFlagBits::eTransferSrc
    );
    text_renderer.text_texture_image = Image::create_2d(
        device, 1024, 1024, 1, 1, vk::Format::eR8G8B8A8Srgb,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );
    text_renderer.text_texture_image_view = ImageView::create_2d(
        device, text_renderer.text_texture_image, vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor
    );
    // transition image layout
    auto command_buffer = CommandBuffer::allocate_primary(device, command_pool);
    command_buffer->begin(vk::CommandBufferBeginInfo{});
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(vk::ImageLayout::eUndefined);
    barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(text_renderer.text_texture_image);
    barrier.setSubresourceRange(
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );
    barrier.setSrcAccessMask({});
    barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
    command_buffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier
    );
    command_buffer->end();
    auto submit_info = vk::SubmitInfo().setCommandBuffers(*command_buffer);
    Fence fence = Fence::create(device, vk::FenceCreateInfo());
    queue->submit(submit_info, fence);
    device->waitForFences(*fence, VK_TRUE, UINT64_MAX);
    fence.destroy(device);
    command_buffer.free(device, command_pool);

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
    std::array<vk::VertexInputAttributeDescription, 3> attribute_descriptions =
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
             .setOffset(offsetof(TextVertex, color))};
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
    rasterizer_info.setCullMode(vk::CullModeFlagBits::eBack);
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

    command.spawn(text_renderer);
}
void destroy_renderer(
    Query<Get<Device>, With<RenderContext>> query,
    Resource<FT2Library> ft2_library,
    Query<Get<TextRenderer>> text_renderer_query
) {
    if (!text_renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device] = query.single().value();
    auto [text_renderer] = text_renderer_query.single().value();
    logger->debug("destroy text renderer");
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
    text_renderer.text_texture_staging_buffer.destroy(device);
    text_renderer.text_texture_image_view.destroy(device);
    text_renderer.text_texture_image.destroy(device);
    FT_Done_FreeType(ft2_library->library);
}
}  // namespace systems
}  // namespace font
}  // namespace pixel_engine