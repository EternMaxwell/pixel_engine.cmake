#pragma once

#include <unordered_map>

#include "vulkan/device.h"
#include "vulkan_headers.h"

namespace pixel_engine {
namespace render_vk {
namespace components {
struct Device {
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debug_messenger;
    vk::PhysicalDevice physical_device;
    vk::Device logical_device;
    VmaAllocator allocator;
    uint32_t queue_family_index;
    vk::Queue queue;
    vk::CommandPool command_pool;
};

struct InFlightFrame {
    vk::Semaphore image_available;
    vk::Semaphore render_finished;
    vk::Fence in_flight;
};

struct Frames {
    std::vector<InFlightFrame> frames = std::vector<InFlightFrame>(1);
    size_t current_frame = 0;
    InFlightFrame& current();
};

struct Swapchain {
    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapchain;
    std::vector<vk::ImageView> image_views;
    vk::SurfaceFormatKHR surface_format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
};

struct RenderGraph;
struct RenderPass;

enum class RenderResourceType {
    Buffer,
    Image,
    Texture,
    Sampler,
};

enum class RenderPassType {
    Compute,
    Graphics,
    Transfer,
};

struct RenderResource {
    struct PassInfo {
        std::weak_ptr<RenderPass> pass;
        uint32_t set;
        uint32_t binding;
        vk::PipelineStageFlagBits stages;
        vk::AccessFlagBits access_flags;
    };

    std::vector<PassInfo> passes_access;
    virtual void read_barrier(vk::CommandBuffer&) = 0;
    virtual void write_barrier(vk::CommandBuffer&) = 0;
    virtual bool need_rebind() = 0;
    virtual bool changed() = 0;
};

struct RenderBufferResource : RenderResource {
    vk::Buffer buffer;
    VmaAllocation allocation;
    vk::DeviceSize size;
    void read_barrier(vk::CommandBuffer& cmd) override {
        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eAllGraphics |
                vk::PipelineStageFlagBits::eTransfer |
                vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eAllGraphics |
                vk::PipelineStageFlagBits::eTransfer |
                vk::PipelineStageFlagBits::eComputeShader,
            {}, {},
            {vk::BufferMemoryBarrier(
                vk::AccessFlagBits::eTransferWrite |
                    vk::AccessFlagBits::eShaderWrite,
                vk::AccessFlagBits::eTransferRead |
                    vk::AccessFlagBits::eShaderRead,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, buffer, 0,
                size
            )},
            {}
        );
    }
    void write_barrier(vk::CommandBuffer& cmd) override {
        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eAllGraphics |
                vk::PipelineStageFlagBits::eTransfer |
                vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eAllGraphics |
                vk::PipelineStageFlagBits::eTransfer |
                vk::PipelineStageFlagBits::eComputeShader,
            {}, {},
            {vk::BufferMemoryBarrier(
                vk::AccessFlagBits::eTransferRead |
                    vk::AccessFlagBits::eShaderRead,
                vk::AccessFlagBits::eTransferWrite |
                    vk::AccessFlagBits::eShaderWrite,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, buffer, 0,
                size
            )},
            {}
        );
    }
    bool need_rebind() override;
    bool changed() override;
};

struct RenderPass {
    struct ResourceInfo {
        std::weak_ptr<RenderResource> resource;
        uint32_t set;
        uint32_t binding;
        vk::PipelineStageFlagBits stages;
        vk::AccessFlagBits access_flags;
    };
    std::vector<ResourceInfo> read_resources;
    std::vector<ResourceInfo> write_resources;
    std::vector<ResourceInfo> read_write_resources;
    virtual void read_storage_buffer(uint32_t, uint32_t, vk::Buffer) = 0;
    virtual void write_storage_buffer(uint32_t, uint32_t, vk::Buffer) = 0;
    virtual void storage_buffer(uint32_t, uint32_t, vk::Buffer) = 0;
    virtual void read_uniform_buffer(uint32_t, uint32_t, vk::Buffer) = 0;
    virtual void write_uniform_buffer(uint32_t, uint32_t, vk::Buffer) = 0;
    virtual void uniform_buffer(uint32_t, uint32_t, vk::Buffer) = 0;
    virtual void read_texture(uint32_t, uint32_t, vk::ImageView) = 0;
    virtual void read_image(uint32_t, uint32_t, vk::ImageView) = 0;
    virtual void write_image(uint32_t, uint32_t, vk::ImageView) = 0;
    virtual void image(uint32_t, uint32_t, vk::ImageView) = 0;
    virtual RenderPassType type() = 0;
};

struct GraphPass : RenderPass {
    void begin(vk::CommandBuffer& cmd, Device& device) {
        for (auto& res_info : read_resources) {
            res_info.resource.lock()->read_barrier(cmd);
        }
    }
    void end(vk::CommandBuffer&, Device&);
    void output_color_attachment(uint32_t, vk::ImageView);
    void output_depth_attachment(vk::ImageView);
    RenderPassType type() override { return RenderPassType::Graphics; }
};

struct RenderGraph;

struct RenderGraph {
    std::vector<std::shared_ptr<RenderPass>> passes;
    std::vector<std::shared_ptr<RenderResource>> resources;
    std::unordered_map<std::string, size_t> resource_map;
    std::unordered_map<std::string, size_t> pass_map;
    Device device;

    struct GraphPassCreateInfo {
        RenderGraph& graph;
        std::string name;

        GraphPassCreateInfo(RenderGraph& graph) : graph(graph) {}

        std::weak_ptr<GraphPass> create() {
            size_t index = graph.passes.size();
            auto pass_ptr = std::make_shared<GraphPass>();
            graph.passes.push_back(pass_ptr);
            // todo
            return pass_ptr;
        }
    };

    GraphPassCreateInfo add_render_pass(std::string name) {
        return GraphPassCreateInfo(*this);
    }
    template <typename T = RenderPass>
    std::shared_ptr<T> find_pass(std::string name) {
        return passes[pass_map[name]];
    }
};

struct RenderVKContext {};
}  // namespace components
}  // namespace render_vk
}  // namespace pixel_engine