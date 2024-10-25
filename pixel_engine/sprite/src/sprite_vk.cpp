#include <stb_image.h>

#include <filesystem>

#include "pixel_engine/sprite.h"
#include "pixel_engine/sprite/shaders/fragment_shader.h"
#include "pixel_engine/sprite/shaders/vertex_shader.h"

using namespace pixel_engine::sprite;
using namespace pixel_engine::sprite::components;
using namespace pixel_engine::sprite::resources;
using namespace pixel_engine::sprite::systems;
using namespace pixel_engine::prelude;
using namespace pixel_engine::render_vk::components;

void systems::create_sprite_renderer_vk(
    Command cmd,
    Query<Get<Device, CommandPool, Queue>, With<RenderContext>> ctx_query
) {
    if (!ctx_query.single().has_value()) return;
    auto [device, command_pool, queue] = ctx_query.single().value();
    vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
    vk::DescriptorSetLayoutBinding descriptor_set_layout_bindings[4] = {
        vk::DescriptorSetLayoutBinding()
            .setBinding(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex),
        vk::DescriptorSetLayoutBinding()
            .setBinding(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex),
        vk::DescriptorSetLayoutBinding()
            .setBinding(2)
            .setDescriptorType(vk::DescriptorType::eSampledImage)
            .setDescriptorCount(65536)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment),
        vk::DescriptorSetLayoutBinding()
            .setBinding(3)
            .setDescriptorType(vk::DescriptorType::eSampler)
            .setDescriptorCount(256)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
    };
    descriptor_set_layout_create_info.setBindings(descriptor_set_layout_bindings
    );
    vk::DescriptorBindingFlags descriptor_binding_flags[4] = {
        {},
        vk::DescriptorBindingFlagBits::ePartiallyBound |
            vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::ePartiallyBound |
            vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::ePartiallyBound |
            vk::DescriptorBindingFlagBits::eUpdateAfterBind
    };
    vk::DescriptorSetLayoutBindingFlagsCreateInfo
        descriptor_set_layout_binding_flags_create_info;
    descriptor_set_layout_binding_flags_create_info.setBindingFlags(
        descriptor_binding_flags
    );
    descriptor_set_layout_create_info
        .setPNext(&descriptor_set_layout_binding_flags_create_info)
        .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
    DescriptorSetLayout sprite_descriptor_set_layout =
        DescriptorSetLayout::create(device, descriptor_set_layout_create_info);
    vk::DescriptorPoolSize descriptor_pool_sizes[4] = {
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(1),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eSampledImage)
            .setDescriptorCount(65536),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eSampler)
            .setDescriptorCount(256)
    };
    vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
    descriptor_pool_create_info.setPoolSizes(descriptor_pool_sizes)
        .setMaxSets(1)
        .setFlags(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet |
            vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind
        );
    DescriptorPool sprite_descriptor_pool =
        DescriptorPool::create(device, descriptor_pool_create_info);
    DescriptorSet sprite_descriptor_set = DescriptorSet::create(
        device, sprite_descriptor_pool, sprite_descriptor_set_layout
    );

    vk::RenderPassCreateInfo render_pass_create_info;
    vk::AttachmentDescription color_attachment;
    color_attachment.setFormat(vk::Format::eB8G8R8A8Srgb)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eLoad)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference color_attachment_ref;
    color_attachment_ref.setAttachment(0).setLayout(
        vk::ImageLayout::eColorAttachmentOptimal
    );
    vk::AttachmentDescription depth_attachment;
    depth_attachment.setFormat(vk::Format::eD32Sfloat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eLoad)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::AttachmentReference depth_attachment_ref;
    depth_attachment_ref.setAttachment(1).setLayout(
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(color_attachment_ref)
        .setPDepthStencilAttachment(&depth_attachment_ref);
    auto attachment_descriptions = {color_attachment, depth_attachment};
    render_pass_create_info.setAttachmentCount(2)
        .setAttachments(attachment_descriptions)
        .setSubpassCount(1)
        .setPSubpasses(&subpass);
    vk::SubpassDependency subpass_dependency;
    subpass_dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eNoneKHR)
        .setDstAccessMask(
            vk::AccessFlagBits::eColorAttachmentRead |
            vk::AccessFlagBits::eColorAttachmentWrite
        );
    render_pass_create_info.setDependencies(subpass_dependency);
    RenderPass sprite_render_pass =
        RenderPass::create(device, render_pass_create_info);

    vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
    pipeline_layout_create_info.setSetLayoutCount(1).setSetLayouts(
        *sprite_descriptor_set_layout
    );
    PipelineLayout sprite_pipeline_layout =
        PipelineLayout::create(device, pipeline_layout_create_info);

    vk::ShaderModuleCreateInfo vert_shader_module_info;
    vert_shader_module_info.setCode(sprite_vk_vertex_spv);
    ShaderModule vert_shader_module =
        ShaderModule::create(device, vert_shader_module_info);
    vk::ShaderModuleCreateInfo frag_shader_module_info;
    frag_shader_module_info.setCode(sprite_vk_fragment_spv);
    ShaderModule frag_shader_module =
        ShaderModule::create(device, frag_shader_module_info);
    std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(vert_shader_module)
            .setPName("main"),
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(frag_shader_module)
            .setPName("main")
    };
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state;
    input_assembly_state.setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(VK_FALSE);
    vk::PipelineDynamicStateCreateInfo dynamic_state;
    std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor
    };
    dynamic_state.setDynamicStates(dynamic_states);
    vk::PipelineRasterizationStateCreateInfo rasterization_state;
    rasterization_state.setDepthClampEnable(VK_FALSE)
        .setRasterizerDiscardEnable(VK_FALSE)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setCullMode(vk::CullModeFlagBits::eNone)
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setDepthBiasEnable(VK_FALSE);
    vk::PipelineColorBlendAttachmentState color_blend_attachment;
    color_blend_attachment.setColorWriteMask(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );
    color_blend_attachment.setBlendEnable(VK_TRUE);
    color_blend_attachment.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);
    vk::PipelineColorBlendStateCreateInfo color_blend_state;
    color_blend_state.setLogicOpEnable(VK_FALSE)
        .setLogicOp(vk::LogicOp::eCopy)
        .setAttachments(color_blend_attachment)
        .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f});
    vk::PipelineMultisampleStateCreateInfo multisample_state;
    multisample_state.setSampleShadingEnable(VK_FALSE).setRasterizationSamples(
        vk::SampleCountFlagBits::e1
    );
    vk::PipelineViewportStateCreateInfo viewport_state;
    viewport_state.setViewportCount(1).setScissorCount(1);
    vk::PipelineVertexInputStateCreateInfo vertex_input_state;
    vk::VertexInputBindingDescription binding_description;
    binding_description.setBinding(0)
        .setStride(sizeof(SpriteVertex))
        .setInputRate(vk::VertexInputRate::eVertex);
    std::array<vk::VertexInputAttributeDescription, 6> attribute_descriptions =
        {vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(0)
             .setFormat(vk::Format::eR32G32B32Sfloat)
             .setOffset(offsetof(SpriteVertex, pos)),
         vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(1)
             .setFormat(vk::Format::eR32G32Sfloat)
             .setOffset(offsetof(SpriteVertex, tex_coord)),
         vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(2)
             .setFormat(vk::Format::eR32G32B32A32Sfloat)
             .setOffset(offsetof(SpriteVertex, color)),
         vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(3)
             .setFormat(vk::Format::eR32Sint)
             .setOffset(offsetof(SpriteVertex, model_index)),
         vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(4)
             .setFormat(vk::Format::eR32Sint)
             .setOffset(offsetof(SpriteVertex, image_index)),
         vk::VertexInputAttributeDescription()
             .setBinding(0)
             .setLocation(5)
             .setFormat(vk::Format::eR32Sint)
             .setOffset(offsetof(SpriteVertex, sampler_index))};
    vertex_input_state.setVertexBindingDescriptions({binding_description})
        .setVertexAttributeDescriptions(attribute_descriptions);
    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
    depth_stencil_state.setDepthTestEnable(VK_TRUE)
        .setDepthWriteEnable(VK_TRUE)
        .setDepthCompareOp(vk::CompareOp::eLess)
        .setDepthBoundsTestEnable(VK_FALSE)
        .setStencilTestEnable(VK_FALSE);
    vk::GraphicsPipelineCreateInfo pipeline_create_info;
    pipeline_create_info.setStages(shader_stages)
        .setPVertexInputState(&vertex_input_state)
        .setPInputAssemblyState(&input_assembly_state)
        .setPViewportState(&viewport_state)
        .setPRasterizationState(&rasterization_state)
        .setPMultisampleState(&multisample_state)
        .setPColorBlendState(&color_blend_state)
        .setPDepthStencilState(&depth_stencil_state)
        .setPDynamicState(&dynamic_state)
        .setLayout(sprite_pipeline_layout)
        .setRenderPass(sprite_render_pass)
        .setSubpass(0);
    Pipeline sprite_pipeline = Pipeline::create(device, pipeline_create_info);
    vert_shader_module.destroy(device);
    frag_shader_module.destroy(device);

    Buffer sprite_uniform_buffer = Buffer::create_host(
        device, sizeof(glm::mat4) * 2, vk::BufferUsageFlagBits::eUniformBuffer
    );
    Buffer sprite_vertex_buffer = Buffer::create_device(
        device, sizeof(SpriteVertex) * 65536,
        vk::BufferUsageFlagBits::eVertexBuffer
    );
    Buffer sprite_model_buffer = Buffer::create_device(
        device, sizeof(glm::mat4) * 65536,
        vk::BufferUsageFlagBits::eStorageBuffer
    );

    SpriteRenderer sprite_renderer;
    sprite_renderer.sprite_descriptor_set_layout = sprite_descriptor_set_layout;
    sprite_renderer.sprite_descriptor_pool = sprite_descriptor_pool;
    sprite_renderer.sprite_descriptor_set = sprite_descriptor_set;
    sprite_renderer.sprite_render_pass = sprite_render_pass;
    sprite_renderer.sprite_pipeline_layout = sprite_pipeline_layout;
    sprite_renderer.sprite_pipeline = sprite_pipeline;
    sprite_renderer.sprite_uniform_buffer = sprite_uniform_buffer;
    sprite_renderer.sprite_vertex_buffer = sprite_vertex_buffer;
    sprite_renderer.sprite_model_buffer = sprite_model_buffer;
    cmd.spawn(sprite_renderer);
}

void systems::destroy_sprite_renderer_vk(
    Query<Get<SpriteRenderer>> renderer_query,
    Query<Get<Device>, With<RenderContext>> query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device] = query.single().value();
    auto [renderer] = renderer_query.single().value();
    renderer.sprite_descriptor_set_layout.destroy(device);
    renderer.sprite_descriptor_pool.destroy(device);
    renderer.sprite_render_pass.destroy(device);
    renderer.sprite_pipeline_layout.destroy(device);
    renderer.sprite_pipeline.destroy(device);
    renderer.sprite_uniform_buffer.destroy(device);
    renderer.sprite_vertex_buffer.destroy(device);
    renderer.sprite_model_buffer.destroy(device);
}