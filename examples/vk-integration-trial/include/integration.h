#include <pixel_engine/prelude.h>
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

#include <random>

#include "fragment_shader.h"
#include "vertex_shader.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::window;
using namespace pixel_engine::render_vk::components;
using namespace pixel_engine::sprite::components;
using namespace pixel_engine::sprite::resources;

namespace vk_trial {

struct Test_T {};

struct Renderer {
    RenderPass render_pass;
    Pipeline graphics_pipeline;
    PipelineLayout pipeline_layout;
    Buffer vertex_buffer;
    Buffer index_buffer;
    VmaAllocation index_buffer_allocation;
    DescriptorSetLayout descriptor_set_layout;
    DescriptorPool descriptor_pool;

    vk::Extent2D current_extent;

    Image texture_image;
    ImageView texture_image_view;
    Sampler texture_sampler;

    Image depth_image;
    ImageView depth_image_view;

    std::vector<DescriptorSet> descriptor_sets;
    std::vector<Buffer> uniform_buffers;
    std::vector<void *> uniform_buffer_data;

    static vk::DescriptorSetLayoutBinding getDescriptorSetLayoutBinding() {
        return vk::DescriptorSetLayoutBinding(
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex
        );
    }

    static vk::DescriptorSetLayoutBinding getTextureDescriptorSetLayoutBinding(
    ) {
        return vk::DescriptorSetLayoutBinding(
            1, vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment
        );
    }
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription(
            0, sizeof(Vertex), vk::VertexInputRate::eVertex
        );
    }

    static std::array<vk::VertexInputAttributeDescription, 3>
    getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)
            ),
            vk::VertexInputAttributeDescription(
                1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)
            ),
            vk::VertexInputAttributeDescription(
                2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)
            )
        };
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

static const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

void create_vertex_buffer(
    Query<Get<Device, Queue, CommandPool>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device, queue, command_pool] = query.single().value();
    auto [renderer]                    = renderer_query.single().value();
    auto &vertices                     = vk_trial::vertices;
    spdlog::info("create vertex buffer");
    vk::BufferCreateInfo buffer_create_info(
        {}, vertices.size() * sizeof(Vertex),
        vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
        vk::SharingMode::eExclusive
    );
    AllocationCreateInfo alloc_info;
    alloc_info.setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    Buffer buffer = Buffer::create(device, buffer_create_info, alloc_info);

    vk::BufferCreateInfo staging_buffer_create_info(
        {}, vertices.size() * sizeof(Vertex),
        vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive
    );
    AllocationCreateInfo staging_alloc_info;
    staging_alloc_info.setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
    staging_alloc_info.setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
    Buffer staging_buffer =
        Buffer::create(device, staging_buffer_create_info, staging_alloc_info);

    void *data = staging_buffer.map(device);
    memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
    staging_buffer.unmap(device);

    CommandBuffer command_buffer =
        CommandBuffer::allocate_primary(device, command_pool);
    command_buffer->begin(vk::CommandBufferBeginInfo{});
    vk::BufferCopy copy_region(0, 0, vertices.size() * sizeof(Vertex));
    command_buffer->copyBuffer(staging_buffer, buffer, copy_region);
    command_buffer->end();
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(*command_buffer);
    queue->submit(submit_info, nullptr);
    queue->waitIdle();
    device->freeCommandBuffers(command_pool, *command_buffer);
    staging_buffer.destroy(device);

    renderer.vertex_buffer = buffer;
}

void free_vertex_buffer(
    Query<Get<Renderer>> renderer_query,
    Query<Get<Device>, With<RenderContext>> query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto [device]   = query.single().value();
    spdlog::info("free vertex buffer");
    renderer.vertex_buffer.destroy(device);
}

void create_index_buffer(
    Query<Get<Renderer>> renderer_query,
    Resource<RenderContext> render_context,
    Query<Get<Device, Queue, CommandPool>, With<RenderContext>> query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device, queue, command_pool] = query.single().value();
    auto [renderer]                    = renderer_query.single().value();
    auto &indices                      = vk_trial::indices;
    spdlog::info("create index buffer");
    vk::BufferCreateInfo buffer_create_info(
        {}, indices.size() * sizeof(uint16_t),
        vk::BufferUsageFlagBits::eIndexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
        vk::SharingMode::eExclusive
    );
    AllocationCreateInfo alloc_info;
    alloc_info.setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    Buffer buffer = Buffer::create(device, buffer_create_info, alloc_info);

    vk::BufferCreateInfo staging_buffer_create_info(
        {}, indices.size() * sizeof(uint16_t),
        vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive
    );
    AllocationCreateInfo staging_alloc_info;
    staging_alloc_info.setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
    staging_alloc_info.setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
    Buffer staging_buffer =
        Buffer::create(device, staging_buffer_create_info, staging_alloc_info);

    void *data = staging_buffer.map(device);
    memcpy(data, indices.data(), indices.size() * sizeof(uint16_t));
    staging_buffer.unmap(device);

    CommandBuffer command_buffer =
        CommandBuffer::allocate_primary(device, command_pool);
    command_buffer->begin(vk::CommandBufferBeginInfo{});
    vk::BufferCopy copy_region(0, 0, indices.size() * sizeof(uint16_t));
    command_buffer->copyBuffer(staging_buffer, buffer, copy_region);
    command_buffer->end();
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(*command_buffer);
    queue->submit(submit_info, nullptr);
    queue->waitIdle();
    command_buffer.free(device, command_pool);

    staging_buffer.destroy(device);

    renderer.index_buffer = buffer;
}

void free_index_buffer(
    Query<Get<Renderer>> renderer_query,
    Resource<RenderContext> render_context,
    Query<Get<Device>, With<RenderContext>> query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("free index buffer");
    renderer.index_buffer.destroy(device);
}

void create_descriptor_set_layout(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query,
    Resource<RenderContext> render_context
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("create descriptor set layout");
    vk::DescriptorSetLayoutBinding ubo_layout_binding =
        Renderer::getDescriptorSetLayoutBinding();
    vk::DescriptorSetLayoutBinding texture_layout_binding =
        Renderer::getTextureDescriptorSetLayoutBinding();
    auto bindings = {ubo_layout_binding, texture_layout_binding};
    vk::DescriptorSetLayoutCreateInfo layout_info;
    layout_info.setBindings(bindings);
    DescriptorSetLayout descriptor_set_layout =
        DescriptorSetLayout::create(device, layout_info);
    renderer.descriptor_set_layout = descriptor_set_layout;
}

void destroy_descriptor_set_layout(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query,
    Resource<RenderContext> render_context
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("destroy descriptor set layout");
    renderer.descriptor_set_layout.destroy(device);
}

void create_uniform_buffers(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query,
    Resource<RenderContext> render_context
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("create uniform buffers");
    size_t buffer_size = sizeof(UniformBufferObject);
    renderer.uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    renderer.uniform_buffer_data.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::BufferCreateInfo buffer_create_info(
            {}, buffer_size, vk::BufferUsageFlagBits::eUniformBuffer,
            vk::SharingMode::eExclusive
        );
        AllocationCreateInfo alloc_info;
        alloc_info.setUsage(VMA_MEMORY_USAGE_AUTO);
        alloc_info.setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
        Buffer buffer = Buffer::create(device, buffer_create_info, alloc_info);

        renderer.uniform_buffers[i]     = buffer;
        renderer.uniform_buffer_data[i] = buffer.map(device);
    }
}

void destroy_uniform_buffers(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query,
    Resource<RenderContext> render_context
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("destroy uniform buffers");
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        renderer.uniform_buffers[i].unmap(device);
        renderer.uniform_buffers[i].destroy(device);
    }
}

void update_uniform_buffer(
    Query<Get<Swapchain>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query,
    Resource<RenderContext> render_context
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [swap_chain]         = query.single().value();
    auto [renderer]           = renderer_query.single().value();
    auto &uniform_buffer_data = renderer.uniform_buffer_data;
    float time                = glfwGetTime();
    UniformBufferObject ubo;
    ubo.model = glm::rotate(
        glm::mat4(1.0f), (float)time * glm::radians(90.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    ubo.view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        swap_chain.extent.width / static_cast<float>(swap_chain.extent.height),
        0.1f, 10.0f
    );
    ubo.proj[1][1] *= -1;
    memcpy(uniform_buffer_data[0], &ubo, sizeof(ubo));
}

void create_depth_image(
    Query<Get<Device, Swapchain>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!query.single().has_value()) return;
    if (!renderer_query.single().has_value()) return;
    auto [device, swap_chain] = query.single().value();
    auto [renderer]           = renderer_query.single().value();
    vk::Format depth_format   = vk::Format::eD32Sfloat;
    spdlog::info("create depth image");
    vk::ImageCreateInfo image_create_info;
    image_create_info.setImageType(vk::ImageType::e2D);
    image_create_info.setExtent(
        vk::Extent3D(swap_chain.extent.width, swap_chain.extent.height, 1)
    );
    image_create_info.setMipLevels(1);
    image_create_info.setArrayLayers(1);
    image_create_info.setFormat(depth_format);
    image_create_info.setTiling(vk::ImageTiling::eOptimal);
    image_create_info.setInitialLayout(vk::ImageLayout::eUndefined);
    image_create_info.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
    image_create_info.setSamples(vk::SampleCountFlagBits::e1);
    AllocationCreateInfo image_alloc_info = {};
    image_alloc_info.setUsage(VMA_MEMORY_USAGE_GPU_ONLY);
    image_alloc_info.setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    image_alloc_info.setPriority(1.0f);
    Image depth_image =
        Image::create(device, image_create_info, image_alloc_info);
    renderer.depth_image = depth_image;
}

void destroy_depth_image(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!query.single().has_value()) return;
    if (!renderer_query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("destroy depth image");
    renderer.depth_image.destroy(device);
}

void create_depth_image_view(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!query.single().has_value()) return;
    if (!renderer_query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("create depth image view");
    vk::ImageViewCreateInfo view_create_info;
    view_create_info.setImage(renderer.depth_image);
    view_create_info.setViewType(vk::ImageViewType::e2D);
    view_create_info.setFormat(vk::Format::eD32Sfloat);
    view_create_info.setSubresourceRange(
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
    );
    ImageView depth_image_view = ImageView::create(device, view_create_info);
    renderer.depth_image_view  = depth_image_view;
}

void destroy_depth_image_view(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!query.single().has_value()) return;
    if (!renderer_query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("destroy depth image view");
    renderer.depth_image_view.destroy(device);
}

void create_texture_image(
    Query<Get<Device, Queue, CommandPool>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    try {
        if (!renderer_query.single().has_value()) return;
        if (!query.single().has_value()) return;
        auto [renderer]                    = renderer_query.single().value();
        auto [device, queue, command_pool] = query.single().value();
        spdlog::info("create texture image");
        int tex_width, tex_height, tex_channels;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc *pixels = stbi_load(
            "./../assets/textures/test.png", &tex_width, &tex_height,
            &tex_channels, 4
        );
        spdlog::info("size: {}, {}", tex_width, tex_height);
        if (!pixels) {
            throw std::runtime_error("Failed to load texture image!");
        }
        size_t image_size = tex_width * tex_height * 4;
        vk::BufferCreateInfo staging_buffer_create_info(
            {}, image_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive
        );
        AllocationCreateInfo staging_alloc_info;
        staging_alloc_info.setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
        staging_alloc_info.setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
        );
        Buffer staging_buffer = Buffer::create(
            device, staging_buffer_create_info, staging_alloc_info
        );
        void *data = staging_buffer.map(device);
        memcpy(data, pixels, image_size);
        staging_buffer.unmap(device);
        stbi_image_free(pixels);

        vk::ImageCreateInfo image_create_info;
        image_create_info.setImageType(vk::ImageType::e2D);
        image_create_info.setExtent(vk::Extent3D(tex_width, tex_height, 1));
        image_create_info.setMipLevels(1);
        image_create_info.setArrayLayers(1);
        image_create_info.setFormat(vk::Format::eR8G8B8A8Srgb);
        image_create_info.setTiling(vk::ImageTiling::eOptimal);
        image_create_info.setInitialLayout(vk::ImageLayout::eUndefined);
        image_create_info.setUsage(
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled
        );
        image_create_info.setSamples(vk::SampleCountFlagBits::e1);
        AllocationCreateInfo image_alloc_info;
        image_alloc_info.setUsage(VMA_MEMORY_USAGE_GPU_ONLY);
        image_alloc_info.setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        image_alloc_info.setPriority(1.0f);
        Image texture_image =
            Image::create(device, image_create_info, image_alloc_info);
        renderer.texture_image = texture_image;

        // Transition image layout
        vk::CommandBufferAllocateInfo allocate_info(
            command_pool, vk::CommandBufferLevel::ePrimary, 1
        );
        CommandBuffer command_buffer =
            CommandBuffer::allocate_primary(device, command_pool);
        command_buffer->begin(vk::CommandBufferBeginInfo{});
        vk::ImageMemoryBarrier barrier;
        barrier.setOldLayout(vk::ImageLayout::eUndefined);
        barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setImage(texture_image);
        barrier.setSubresourceRange(vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        ));
        barrier.setSrcAccessMask({});
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        command_buffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier
        );
        command_buffer->end();
        vk::SubmitInfo submit_info;
        submit_info.setCommandBuffers(*command_buffer);
        queue->submit(submit_info, nullptr);
        queue->waitIdle();
        command_buffer.free(device, command_pool);
        // Copy buffer to image
        command_buffer = CommandBuffer::allocate_primary(device, command_pool);
        command_buffer->begin(vk::CommandBufferBeginInfo{});
        vk::BufferImageCopy region;
        region.setBufferOffset(0);
        region.setBufferRowLength(0);
        region.setBufferImageHeight(0);
        region.setImageSubresource(
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1)
        );
        region.setImageOffset({0, 0, 0});
        region.setImageExtent({(uint32_t)tex_width, (uint32_t)tex_height, 1u});
        command_buffer->copyBufferToImage(
            staging_buffer, renderer.texture_image,
            vk::ImageLayout::eTransferDstOptimal, region
        );
        command_buffer->end();
        submit_info.setCommandBuffers(*command_buffer);
        queue->submit(submit_info, nullptr);
        queue->waitIdle();
        command_buffer.free(device, command_pool);
        // transition image layout
        command_buffer = CommandBuffer::allocate_primary(device, command_pool);
        command_buffer->begin(vk::CommandBufferBeginInfo{});
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setImage(texture_image);
        barrier.setSubresourceRange(vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        ));
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        command_buffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier
        );
        command_buffer->end();
        submit_info.setCommandBuffers(*command_buffer);
        queue->submit(submit_info, nullptr);
        queue->waitIdle();
        command_buffer.free(device, command_pool);
        staging_buffer.destroy(device);
    } catch (const std::exception &e) {
        spdlog::error("error at create texture image : {}", e.what());
    }
}

void destroy_texture_image(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("destroy texture image");
    renderer.texture_image.destroy(device);
}

void create_texture_image_view(
    Command cmd,
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query,
    Resource<SpriteServerVK> sprite_server
) {
    if (sprite_server.has_value()) {
        cmd.spawn(
            Sprite{
                .image = sprite_server->load_image(
                    cmd, "./../assets/textures/test.png"
                ),
                .sampler = sprite_server->create_sampler(
                    cmd,
                    vk::SamplerCreateInfo()
                        .setMagFilter(vk::Filter::eLinear)
                        .setMinFilter(vk::Filter::eLinear)
                        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                        .setAnisotropyEnable(VK_TRUE)
                        .setMaxAnisotropy(16)
                        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                        .setUnnormalizedCoordinates(VK_FALSE)
                        .setCompareEnable(VK_FALSE)
                        .setCompareOp(vk::CompareOp::eAlways)
                        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                        .setMipLodBias(0.0f)
                        .setMinLod(0.0f)
                        .setMaxLod(0.0f),
                    "test_sampler"
                ),
                .size  = {1200.0f, 1200.0f},
                .color = {1.0f, 1.0f, 1.0f, 0.6f}
            },
            SpritePos2D{.pos = {-400.0f, 0.0f, 10.0f}}
        );
        cmd.spawn(
            Sprite{
                .image = sprite_server->load_image(
                    cmd, "./../assets/textures/test1.jfif"
                ),
                .sampler = sprite_server->create_sampler(
                    cmd,
                    vk::SamplerCreateInfo()
                        .setMagFilter(vk::Filter::eNearest)
                        .setMinFilter(vk::Filter::eNearest)
                        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                        .setAnisotropyEnable(VK_TRUE)
                        .setMaxAnisotropy(16)
                        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                        .setUnnormalizedCoordinates(VK_FALSE)
                        .setCompareEnable(VK_FALSE)
                        .setCompareOp(vk::CompareOp::eAlways)
                        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                        .setMipLodBias(0.0f)
                        .setMinLod(0.0f)
                        .setMaxLod(0.0f),
                    "test_sampler2"
                ),
                .size  = {500.0f, 500.0f},
                .color = {1.0f, 1.0f, 1.0f, 0.6f}
            },
            SpritePos2D{.pos = {400.0f, 0.0f, 5.9f}}
        );
    }
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("create texture image view");
    vk::ImageViewCreateInfo view_info;
    view_info.setImage(renderer.texture_image);
    view_info.setViewType(vk::ImageViewType::e2D);
    view_info.setFormat(vk::Format::eR8G8B8A8Srgb);
    view_info.setSubresourceRange(
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );
    ImageView texture_image_view = ImageView::create(device, view_info);
    renderer.texture_image_view  = texture_image_view;
}

void destroy_texture_image_view(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("destroy texture image view");
    renderer.texture_image_view.destroy(device);
}

void create_texture_image_sampler(
    Query<Get<PhysicalDevice, Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [physical_device, device] = query.single().value();
    auto [renderer]                = renderer_query.single().value();
    spdlog::info("create texture image sampler");
    vk::SamplerCreateInfo sampler_info;
    sampler_info.setMagFilter(vk::Filter::eLinear);
    sampler_info.setMinFilter(vk::Filter::eLinear);
    sampler_info.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAnisotropyEnable(VK_TRUE);
    vk::PhysicalDeviceProperties properties = physical_device->getProperties();
    sampler_info.setMaxAnisotropy(properties.limits.maxSamplerAnisotropy);
    sampler_info.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
    sampler_info.setUnnormalizedCoordinates(VK_FALSE);
    sampler_info.setCompareEnable(VK_FALSE);
    sampler_info.setCompareOp(vk::CompareOp::eAlways);
    sampler_info.setMipmapMode(vk::SamplerMipmapMode::eLinear);
    sampler_info.setMipLodBias(0.0f);
    sampler_info.setMinLod(0.0f);
    sampler_info.setMaxLod(0.0f);
    Sampler texture_sampler  = Sampler::create(device, sampler_info);
    renderer.texture_sampler = texture_sampler;
}

void destroy_texture_image_sampler(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("destroy texture image sampler");
    renderer.texture_sampler.destroy(device);
}

void create_descriptor_pool(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query,
    Resource<RenderContext> render_context
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("create descriptor pool");
    auto pool_sizes = {
        vk::DescriptorPoolSize(
            vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT
        ),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT
        )
    };
    vk::DescriptorPoolCreateInfo pool_info;
    pool_info.setPoolSizes(pool_sizes);
    pool_info.setMaxSets(MAX_FRAMES_IN_FLIGHT);
    DescriptorPool descriptor_pool = DescriptorPool::create(device, pool_info);
    renderer.descriptor_pool       = descriptor_pool;
}

void destroy_descriptor_pool(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]   = query.single().value();
    auto [renderer] = renderer_query.single().value();
    spdlog::info("destroy descriptor pool");
    renderer.descriptor_pool.destroy(device);
}

void create_descriptor_sets(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]               = query.single().value();
    auto [renderer]             = renderer_query.single().value();
    auto &descriptor_set_layout = renderer.descriptor_set_layout;
    auto &descriptor_pool       = renderer.descriptor_pool;
    auto &uniform_buffers       = renderer.uniform_buffers;
    spdlog::info("create descriptor sets");
    std::vector<DescriptorSet> descriptor_sets;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        DescriptorSet descriptor_set = DescriptorSet::create(
            device, descriptor_pool, descriptor_set_layout
        );
        descriptor_sets.push_back(descriptor_set);
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo buffer_info(
            uniform_buffers[i], 0, sizeof(UniformBufferObject)
        );
        vk::DescriptorImageInfo image_info(
            renderer.texture_sampler, renderer.texture_image_view,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
        vk::WriteDescriptorSet descriptor_write;
        descriptor_write.setDstSet(descriptor_sets[i]);
        descriptor_write.setDstBinding(0);
        descriptor_write.setDstArrayElement(0);
        descriptor_write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
        descriptor_write.setDescriptorCount(1);
        descriptor_write.setPBufferInfo(&buffer_info);
        vk::WriteDescriptorSet descriptor_write2;
        descriptor_write2.setDstSet(descriptor_sets[i]);
        descriptor_write2.setDstBinding(1);
        descriptor_write2.setDstArrayElement(0);
        descriptor_write2.setDescriptorType(
            vk::DescriptorType::eCombinedImageSampler
        );
        descriptor_write2.setDescriptorCount(1);
        descriptor_write2.setPImageInfo(&image_info);
        auto descriptor_writes = {descriptor_write, descriptor_write2};
        device->updateDescriptorSets(descriptor_writes, nullptr);
    }
    renderer.descriptor_sets = descriptor_sets;
}

void create_renderer(Command command) { command.spawn(Renderer{}); }

void create_render_pass(
    Query<Get<Device, Swapchain>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device, swap_chain] = query.single().value();
    auto [renderer]           = renderer_query.single().value();

    spdlog::info("create render pass");
    vk::AttachmentDescription color_attachment(
        {}, swap_chain.surface_format.format, vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
    );
    vk::AttachmentDescription depth_attachment(
        {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
    vk::AttachmentReference color_attachment_ref(
        0, vk::ImageLayout::eColorAttachmentOptimal
    );
    vk::AttachmentReference depth_attachment_ref(
        1, vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
    vk::SubpassDescription subpass(
        {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1,
        &color_attachment_ref
    );
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachments(color_attachment_ref);
    subpass.setPDepthStencilAttachment(&depth_attachment_ref);
    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = {};
    dependency.dstStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite |
                               vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    std::vector<vk::AttachmentDescription> attachments = {
        color_attachment, depth_attachment
    };
    vk::RenderPassCreateInfo render_pass_create_info;
    render_pass_create_info.setAttachments(attachments);
    render_pass_create_info.setSubpasses(subpass);
    render_pass_create_info.setDependencies(dependency);
    RenderPass render_pass =
        RenderPass::create(device, render_pass_create_info);
    renderer.render_pass = render_pass;
}

void create_graphics_pipeline(
    Query<Get<Device, Swapchain>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device, swap_chain] = query.single().value();
    auto [renderer]           = renderer_query.single().value();
    auto &render_pass         = renderer.render_pass;
    spdlog::info("create graphics pipeline");
    // Create the shader modules and the shader stages
    auto vertex_code = std::vector<uint32_t>(
        vertex_spv, vertex_spv + sizeof(vertex_spv) / sizeof(uint32_t)
    );
    auto fragment_code = std::vector<uint32_t>(
        fragment_spv, fragment_spv + sizeof(fragment_spv) / sizeof(uint32_t)
    );
    ShaderModule vertex_shader_module = ShaderModule::create(
        device, vk::ShaderModuleCreateInfo({}, vertex_code)
    );
    ShaderModule fragment_shader_module = ShaderModule::create(
        device, vk::ShaderModuleCreateInfo({}, fragment_code)
    );
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
        {{}, vk::ShaderStageFlagBits::eVertex, vertex_shader_module, "main"},
        {{}, vk::ShaderStageFlagBits::eFragment, fragment_shader_module, "main"}
    };
    // create dynamic state
    std::vector<vk::DynamicState> dynamic_states = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state_create_info(
        {}, dynamic_states
    );
    // create vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input_create_info({}, {}, {});
    auto attribute_descriptions = Vertex::getAttributeDescriptions();
    auto binding_description    = Vertex::getBindingDescription();
    vertex_input_create_info.setVertexAttributeDescriptions(
        attribute_descriptions
    );
    vertex_input_create_info.setVertexBindingDescriptions(binding_description);
    // create input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info(
        {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE
    );
    // create viewport
    auto &extent = swap_chain.extent;
    vk::Viewport viewport(
        0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f
    );
    vk::Rect2D scissor({0, 0}, extent);
    vk::PipelineViewportStateCreateInfo viewport_state_create_info(
        {}, 1, nullptr, 1, nullptr
    );
    // create rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer_create_info;
    rasterizer_create_info.setDepthClampEnable(VK_FALSE);
    rasterizer_create_info.setRasterizerDiscardEnable(VK_FALSE);
    rasterizer_create_info.setPolygonMode(vk::PolygonMode::eFill);
    rasterizer_create_info.setLineWidth(1.0f);
    rasterizer_create_info.setCullMode(vk::CullModeFlagBits::eFront);
    rasterizer_create_info.setFrontFace(vk::FrontFace::eClockwise);
    rasterizer_create_info.setDepthBiasEnable(VK_FALSE);
    // create multisampling
    vk::PipelineMultisampleStateCreateInfo multisample_create_info;
    multisample_create_info.sampleShadingEnable  = VK_FALSE;
    multisample_create_info.rasterizationSamples = vk::SampleCountFlagBits::e1;
    // create color blending
    vk::PipelineColorBlendAttachmentState color_blend_attachment;
    color_blend_attachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_FALSE;
    vk::PipelineColorBlendStateCreateInfo color_blend_create_info(
        {}, VK_FALSE, vk::LogicOp::eCopy, color_blend_attachment,
        {0.0f, 0.0f, 0.0f, 0.0f}
    );
    // create pipeline layout
    vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
    pipeline_layout_create_info.setSetLayouts(*(renderer.descriptor_set_layout)
    );
    PipelineLayout pipeline_layout =
        PipelineLayout::create(device, pipeline_layout_create_info);
    renderer.pipeline_layout = pipeline_layout;

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_create_info;
    depth_stencil_create_info.setDepthTestEnable(VK_TRUE);
    depth_stencil_create_info.setDepthWriteEnable(VK_TRUE);
    depth_stencil_create_info.setDepthCompareOp(vk::CompareOp::eLess);
    depth_stencil_create_info.setDepthBoundsTestEnable(VK_FALSE);
    depth_stencil_create_info.setMinDepthBounds(0.0f);
    depth_stencil_create_info.setMaxDepthBounds(1.0f);
    depth_stencil_create_info.setStencilTestEnable(VK_FALSE);

    vk::PipelineRenderingCreateInfo pipeline_render_info;
    pipeline_render_info.setColorAttachmentFormats(
        swap_chain.surface_format.format
    );
    pipeline_render_info.setDepthAttachmentFormat(vk::Format::eD32Sfloat);

    // create pipeline
    vk::GraphicsPipelineCreateInfo create_info;
    create_info.setStages(shader_stages);
    create_info.setPVertexInputState(&vertex_input_create_info);
    create_info.setPInputAssemblyState(&input_assembly_create_info);
    create_info.setPViewportState(&viewport_state_create_info);
    create_info.setPRasterizationState(&rasterizer_create_info);
    create_info.setPMultisampleState(&multisample_create_info);
    create_info.setPColorBlendState(&color_blend_create_info);
    create_info.setPDynamicState(&dynamic_state_create_info);
    create_info.setPDepthStencilState(&depth_stencil_create_info);
    create_info.setLayout(pipeline_layout);
    create_info.setPNext(&pipeline_render_info);
    create_info.setSubpass(0);
    auto pipeline              = Pipeline::create(device, create_info);
    renderer.graphics_pipeline = pipeline;

    vertex_shader_module.destroy(device);
    fragment_shader_module.destroy(device);
}

struct IntegrationTestCmd {};

void record_command_buffer(
    Query<Get<Device, Swapchain>, With<RenderContext>> query,
    Query<Get<Device>, With<RenderContext>> device_query,
    Query<Get<Renderer>> renderer_query,
    Query<Get<CommandBuffer>, With<IntegrationTestCmd>> cmd_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device, swap_chain] = query.single().value();
    auto [renderer]           = renderer_query.single().value();
    auto [cmd]                = cmd_query.single().value();
    auto &render_pass         = renderer.render_pass;
    auto &pipeline            = renderer.graphics_pipeline;
    if (renderer.current_extent != swap_chain.extent) {
        destroy_depth_image_view(device_query, renderer_query);
        destroy_depth_image(device_query, renderer_query);
        create_depth_image(query, renderer_query);
        create_depth_image_view(device_query, renderer_query);
        renderer.current_extent = swap_chain.extent;
    }
    // spdlog::info("record command buffer");
    cmd->begin(vk::CommandBufferBeginInfo{});
    vk::ClearValue clear_color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    vk::ClearValue clear_depth;
    clear_depth.setDepthStencil({1.0f, 0});
    auto clear_values = {clear_color, clear_depth};
    auto depth_attachment_info =
        vk::RenderingAttachmentInfo()
            .setClearValue(clear_depth)
            .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setImageView(renderer.depth_image_view)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eDontCare);
    cmd->beginRendering(
        vk::RenderingInfo()
            .setColorAttachments(
                vk::RenderingAttachmentInfo()
                    .setClearValue(clear_color)
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(swap_chain.current_image_view())
                    .setLoadOp(vk::AttachmentLoadOp::eLoad)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            )
            .setPDepthAttachment(&depth_attachment_info)
            .setRenderArea({{0, 0}, swap_chain.extent})
            .setLayerCount(1)
    );
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    vk::Viewport viewport(
        0.0f, 0.0f, (float)swap_chain.extent.width,
        (float)swap_chain.extent.height, 0.0f, 1.0f
    );
    vk::Rect2D scissor({0, 0}, swap_chain.extent);
    cmd->setViewport(0, viewport);
    cmd->setScissor(0, scissor);
    cmd->bindVertexBuffers(0, *renderer.vertex_buffer, {0});
    cmd->bindIndexBuffer(renderer.index_buffer, 0, vk::IndexType::eUint16);
    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, renderer.pipeline_layout, 0,
        renderer.descriptor_sets[0].descriptor_set, {}
    );
    cmd->drawIndexed(indices.size(), 1, 0, 0, 0);
    cmd->endRendering();
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
    barrier.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(swap_chain.current_image());
    barrier.setSubresourceRange(
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );
    barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
    cmd->pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, barrier
    );
    cmd->end();
}

void begin_draw(
    Command command,
    Query<Get<Device, Swapchain>, With<RenderContext>> query,
    Query<Get<Device>, With<RenderContext>> device_query,
    Query<Get<CommandPool>, With<RenderContext>> command_pool_query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    if (!command_pool_query.single().has_value()) return;
    auto [command_pool]       = command_pool_query.single().value();
    auto [device, swap_chain] = query.single().value();
    auto [renderer]           = renderer_query.single().value();
    auto &in_flight_fence     = swap_chain.fence();
    try {
        auto cmd = CommandBuffer::allocate_primary(device, command_pool);
        command.spawn(cmd, IntegrationTestCmd{});
    } catch (vk::OutOfDateKHRError const &e) {
        spdlog::info("swap chain out of date : {}", e.what());
    }
}

void end_draw(
    Query<Get<Device, Swapchain, Queue>, With<RenderContext>> query,
    Query<Get<Device>, With<RenderContext>> device_query,
    Query<Get<CommandPool>, With<RenderContext>> command_pool_query,
    Query<Get<Renderer>> renderer_query,
    Query<Get<Entity, CommandBuffer>, With<IntegrationTestCmd>> cmd_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    if (!cmd_query.single().has_value()) return;
    if (!command_pool_query.single().has_value()) return;
    auto [command_pool]              = command_pool_query.single().value();
    auto [id, cmd]                   = cmd_query.single().value();
    auto [device, swap_chain, queue] = query.single().value();
    auto [renderer]                  = renderer_query.single().value();
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(*cmd);
    queue->submit(submit_info, nullptr);
    // free cmd
    device->waitIdle();
    cmd.free(device, command_pool);
}

auto draw_frame = std::make_tuple(
    begin_draw, update_uniform_buffer, record_command_buffer, end_draw
);

void destroy_pipeline(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]         = query.single().value();
    auto [renderer]       = renderer_query.single().value();
    auto &pipeline        = renderer.graphics_pipeline;
    auto &pipeline_layout = renderer.pipeline_layout;
    spdlog::info("destroy pipeline");
    pipeline.destroy(device);
    pipeline_layout.destroy(device);
}

void destroy_render_pass(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    if (!query.single().has_value()) return;
    auto [device]     = query.single().value();
    auto [renderer]   = renderer_query.single().value();
    auto &render_pass = renderer.render_pass;
    spdlog::info("destroy render pass");
    render_pass.destroy(device);
}

void wait_for_device(Query<Get<Device>, With<RenderContext>> query) {
    if (!query.single().has_value()) return;
    auto [device] = query.single().value();
    spdlog::info("wait for device");
    device->waitIdle();
}

void create_text(
    Command command,
    Resource<pixel_engine::font::resources::FT2Library> ft2_library
) {
    if (!ft2_library.has_value()) return;
    pixel_engine::font::components::Text text;
    auto font_face =
        ft2_library->load_font("../assets/fonts/FiraSans-Bold.ttf");
    text.font = pixel_engine::font::components::Font{.font_face = font_face};
    text.font.pixels = 32;
    text.text =
        L"Hello, "
        L"Worldajthgreawiohguhiuwearjhoiughjoaewrughiowahioulgerjioweahjgiuawhi"
        L"ohiouaewhoiughjwaoigoiehafgioerhiiUWEGHNVIOAHJEDSKGBHJIUAERWHJIUGOHoa"
        L"ghweiuo ioweafgioewajiojoiatg huihkljh";
    command.spawn(text, pixel_engine::font::components::TextPos{100, 500});
    pixel_engine::font::components::Text text2;
    font_face = ft2_library->load_font(
        "../assets/fonts/HachicroUndertaleBattleFontRegular-L3zlg.ttf"
    );
    text2.font = pixel_engine::font::components::Font{.font_face = font_face};
    text2.font.pixels = 32;
    text2.text =
        L"Hello, "
        L"Worldoawgheruiahjgijanglkjwaehjgo;"
        L"ierwhjgiohnweaioulgfhjewjfweg3ioioiwefiowejhoiewfgjoiweaghioweahioawe"
        L"gHJWEAIOUHAWEFGIOULHJEAWio;hWE$gowaejgio";
    command.spawn(text2, pixel_engine::font::components::TextPos{100, 200});
}

void shuffle_text(Query<Get<pixel_engine::font::components::Text>> text_query) {
    for (auto [text] : text_query.iter()) {
        std::random_device rd;
        std::mt19937 g(rd());
        for (size_t i = 0; i < text.text.size(); i++) {
            std::uniform_int_distribution<> dis(0, text.text.size() - 1);
            text.text[i] = wchar_t(dis(g));
        }
    }
}

using namespace pixel_engine::input::components;

void output_event(
    EventReader<pixel_engine::input::events::MouseScroll> scroll_events,
    Query<Get<ButtonInput<KeyCode>, ButtonInput<MouseButton>, const Window>>
        query,
    EventReader<pixel_engine::input::events::CursorMove> cursor_move_events
) {
    for (auto event : scroll_events.read()) {
        spdlog::info("scroll : {}", event.yoffset);
    }
    for (auto [key_input, mouse_input, window] : query.iter()) {
        for (auto key : key_input.just_pressed_keys()) {
            auto *key_name = key_input.key_name(key);
            if (key_name == nullptr) {
                spdlog::info("key {} just pressed", static_cast<int>(key));
            } else {
                spdlog::info("key {} just pressed", key_name);
            }
        }
        for (auto key : key_input.just_released_keys()) {
            auto *key_name = key_input.key_name(key);
            if (key_name == nullptr) {
                spdlog::info("key {} just released", static_cast<int>(key));
            } else {
                spdlog::info("key {} just released", key_name);
            }
        }
        for (auto button : mouse_input.just_pressed_buttons()) {
            spdlog::info(
                "mouse button {} just pressed", static_cast<int>(button)
            );
        }
        for (auto button : mouse_input.just_released_buttons()) {
            spdlog::info(
                "mouse button {} just released", static_cast<int>(button)
            );
        }
        // if (window.get_cursor_move().has_value()) {
        //     auto [x, y] = window.get_cursor_move().value();
        //     spdlog::info("cursor move -from window : {}, {}", x, y);
        // }
    }
    for (auto event : cursor_move_events.read()) {
        spdlog::info("cursor move -from events : {}, {}", event.x, event.y);
    }
}

struct VK_TrialPlugin : Plugin {
    void build(App &app) override {
        auto window_plugin = app.get_plugin<WindowPlugin>();
        // window_plugin->primary_window_vsync = false;

        using namespace pixel_engine;

        app.enable_loop();
        app.add_system(create_renderer)
            .use_worker("single")
            .in_stage(app::PreStartup);
        app.add_system(create_vertex_buffer)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(create_index_buffer)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(create_uniform_buffers)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(destroy_uniform_buffers)
            .after(wait_for_device)
            .in_stage(app::Shutdown)
            .use_worker("single");
        app.add_system(create_depth_image)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(destroy_depth_image)
            .after(wait_for_device)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(create_depth_image_view)
            .after(create_depth_image)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(destroy_depth_image_view)
            .after(wait_for_device)
            .before(destroy_depth_image)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(create_texture_image)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(destroy_texture_image)
            .after(wait_for_device)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(create_texture_image_view)
            .after(create_texture_image)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(destroy_texture_image_view)
            .after(wait_for_device)
            .before(destroy_texture_image)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(create_texture_image_sampler)
            .after(create_texture_image_view)
            .before(create_descriptor_sets)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(destroy_texture_image_sampler)
            .after(wait_for_device)
            .before(destroy_texture_image_view)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(create_descriptor_set_layout)
            .before(create_descriptor_pool)
            .before(create_graphics_pipeline)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(destroy_descriptor_set_layout)
            .after(wait_for_device)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(create_descriptor_pool)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(destroy_descriptor_pool)
            .use_worker("single")
            .in_stage(app::Shutdown)
            .before(destroy_descriptor_set_layout)
            .after(wait_for_device);
        app.add_system(create_descriptor_sets)
            .after(create_descriptor_pool, create_uniform_buffers)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(create_render_pass)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(create_graphics_pipeline)
            .after(create_render_pass)
            .use_worker("single")
            .in_stage(app::Startup);
        app.add_system(draw_frame)
            .use_worker("single")
            .in_stage(app::Render)
            .before(
                pixel_engine::font::systems::draw_text,
                pixel_engine::sprite::systems::draw_sprite_2d_vk
            );
        app.add_system(wait_for_device)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(free_vertex_buffer)
            .after(wait_for_device)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(free_index_buffer)
            .after(wait_for_device)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(destroy_pipeline)
            .after(wait_for_device)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(destroy_render_pass)
            .after(wait_for_device)
            .use_worker("single")
            .in_stage(app::Shutdown);
        app.add_system(create_text).in_stage(app::Startup);
        app.add_system(shuffle_text).in_stage(app::Update);
        app.add_system(output_event).in_stage(app::Update);
    }
};

void run() {
    App app;
    app.set_run_time_rate(1.0);
    app.log_level(App::Loggers::Build, spdlog::level::debug);
    app.enable_loop();
    app.add_plugin(pixel_engine::window::WindowPlugin{});
    app.add_plugin(pixel_engine::input::InputPlugin{});
    app.add_plugin(pixel_engine::render_vk::RenderVKPlugin{});
    app.add_plugin(pixel_engine::font::FontPlugin{});
    app.add_plugin(vk_trial::VK_TrialPlugin{});
    app.add_plugin(pixel_engine::render::pixel::PixelRenderPlugin{});
    app.add_plugin(pixel_engine::sprite::SpritePluginVK{});
    app.run();
}
}  // namespace vk_trial