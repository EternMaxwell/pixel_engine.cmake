#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <unordered_map>
#include <vector>

#include "vulkan/device.h"
#include "vulkan_headers.h"

namespace pixel_engine {
namespace render_vk {
namespace components {
struct RenderContext {};

struct Instance {
    void destroy();
    operator vk::Instance() const;
    operator VkInstance() const;
    bool operator!() const;
    vk::Instance* operator->();
    vk::Instance& operator*();

    static Instance create(
        const char* app_name,
        uint32_t app_version,
        std::shared_ptr<spdlog::logger> logger
    );

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debug_messenger;
    std::shared_ptr<spdlog::logger> logger;
};

struct PhysicalDevice {
    operator vk::PhysicalDevice() const;
    operator VkPhysicalDevice() const;
    bool operator!() const;
    vk::PhysicalDevice* operator->();
    vk::PhysicalDevice& operator*();

    static PhysicalDevice create(Instance instance);

    vk::PhysicalDevice physical_device;
};

struct Device {
    void destroy();
    operator vk::Device() const;
    operator VkDevice() const;
    bool operator!() const;
    vk::Device* operator->();
    vk::Device& operator*();

    static Device create(
        Instance& instance,
        PhysicalDevice& physical_device,
        vk::QueueFlags queue_flags = vk::QueueFlagBits::eGraphics
    );

    vk::Device device;
    VmaAllocator allocator;
    uint32_t queue_family_index;
};

struct Queue {
    operator vk::Queue() const;
    operator VkQueue() const;
    bool operator!() const;
    vk::Queue* operator->();
    vk::Queue& operator*();

    static Queue create(Device& device);

    vk::Queue queue;
};

struct CommandPool {
    void destroy(Device device);
    operator vk::CommandPool() const;
    operator VkCommandPool() const;
    bool operator!() const;
    vk::CommandPool* operator->();
    vk::CommandPool& operator*();

    static CommandPool create(Device& device);

    vk::CommandPool command_pool;
};

struct CommandBuffer {
    void free(Device& device, CommandPool& command_pool);
    operator vk::CommandBuffer() const;
    operator VkCommandBuffer() const;
    bool operator!() const;
    vk::CommandBuffer* operator->();
    vk::CommandBuffer& operator*();

    static CommandBuffer allocate_primary(
        Device& device, CommandPool& command_pool
    );
    static CommandBuffer allocate_secondary(
        Device& device, CommandPool& command_pool
    );

    vk::CommandBuffer command_buffer;
};

struct AllocationCreateInfo {
    AllocationCreateInfo();

    operator VmaAllocationCreateInfo() const;
    AllocationCreateInfo& setUsage(VmaMemoryUsage usage);
    AllocationCreateInfo& setFlags(VmaAllocationCreateFlags flags);
    AllocationCreateInfo& setRequiredFlags(VkMemoryPropertyFlags flags);
    AllocationCreateInfo& setPreferredFlags(VkMemoryPropertyFlags flags);
    AllocationCreateInfo& setMemoryTypeBits(uint32_t bits);
    AllocationCreateInfo& setPool(VmaPool pool);
    AllocationCreateInfo& setPUserData(void* data);
    AllocationCreateInfo& setPriority(float priority);
    VmaAllocationCreateInfo& operator*();

    VmaAllocationCreateInfo create_info;
};

struct Buffer {
    void destroy(Device& device);
    void* map(Device& device);
    void unmap(Device& device);
    operator vk::Buffer() const;
    operator VkBuffer() const;
    bool operator!() const;
    vk::Buffer* operator->();
    vk::Buffer& operator*();

    static Buffer create(
        Device& device,
        vk::BufferCreateInfo create_info,
        AllocationCreateInfo alloc_info
    );

    vk::Buffer buffer;
    VmaAllocation allocation;
};

struct Image {
    void destroy(Device& device);
    operator vk::Image() const;
    operator VkImage() const;
    bool operator!() const;
    vk::Image* operator->();
    vk::Image& operator*();

    static Image create(
        Device& device,
        vk::ImageCreateInfo create_info,
        AllocationCreateInfo alloc_info
    );

    vk::Image image;
    VmaAllocation allocation;
};

struct ImageView {
    void destroy(Device& device);
    operator vk::ImageView() const;
    operator VkImageView() const;
    bool operator!() const;
    vk::ImageView* operator->();
    vk::ImageView& operator*();

    static ImageView create(
        Device& device, vk::ImageViewCreateInfo create_info
    );

    vk::ImageView image_view;
};

struct Sampler {
    void destroy(Device& device);
    operator vk::Sampler() const;
    operator VkSampler() const;
    bool operator!() const;
    vk::Sampler* operator->();
    vk::Sampler& operator*();

    static Sampler create(Device& device, vk::SamplerCreateInfo create_info);

    vk::Sampler sampler;
};

struct DescriptorSetLayout {
    void destroy(Device& device);
    operator vk::DescriptorSetLayout() const;
    operator VkDescriptorSetLayout() const;
    bool operator!() const;
    vk::DescriptorSetLayout* operator->();
    vk::DescriptorSetLayout& operator*();

    static DescriptorSetLayout create(
        Device& device, vk::DescriptorSetLayoutCreateInfo create_info
    );

    vk::DescriptorSetLayout descriptor_set_layout;
};

struct PipelineLayout {
    void destroy(Device& device);
    operator vk::PipelineLayout() const;
    operator VkPipelineLayout() const;
    bool operator!() const;
    vk::PipelineLayout* operator->();
    vk::PipelineLayout& operator*();

    static PipelineLayout create(
        Device& device, vk::PipelineLayoutCreateInfo create_info
    );

    vk::PipelineLayout pipeline_layout;
};

struct Pipeline {
    void destroy(Device& device);
    operator vk::Pipeline() const;
    operator VkPipeline() const;
    bool operator!() const;
    vk::Pipeline* operator->();
    vk::Pipeline& operator*();

    static Pipeline create(
        Device& device,
        vk::PipelineCache pipeline_cache,
        vk::GraphicsPipelineCreateInfo create_info
    );
    static Pipeline create(
        Device& device, vk::GraphicsPipelineCreateInfo create_info
    );

    vk::Pipeline pipeline;
};

struct DescriptorPool {
    void destroy(Device& device);
    operator vk::DescriptorPool() const;
    operator VkDescriptorPool() const;
    bool operator!() const;
    vk::DescriptorPool* operator->();
    vk::DescriptorPool& operator*();

    static DescriptorPool create(
        Device& device, vk::DescriptorPoolCreateInfo create_info
    );

    vk::DescriptorPool descriptor_pool;
};

struct DescriptorSet {
    void destroy(Device& device, DescriptorPool& descriptor_pool);
    operator vk::DescriptorSet() const;
    operator VkDescriptorSet() const;
    bool operator!() const;
    vk::DescriptorSet* operator->();
    vk::DescriptorSet& operator*();

    static DescriptorSet create(
        Device& device, vk::DescriptorSetAllocateInfo allocate_info
    );
    static DescriptorSet create(
        Device& device,
        DescriptorPool& descriptor_pool,
        vk::DescriptorSetLayout descriptor_set_layout
    );

    vk::DescriptorSet descriptor_set;
};

struct Fence {
    void destroy(Device& device);
    operator vk::Fence() const;
    operator VkFence() const;
    bool operator!() const;
    vk::Fence* operator->();
    vk::Fence& operator*();

    static Fence create(Device& device, vk::FenceCreateInfo create_info);

    vk::Fence fence;
};

struct Semaphore {
    void destroy(Device& device);
    operator vk::Semaphore() const;
    operator VkSemaphore() const;
    bool operator!() const;
    vk::Semaphore* operator->();
    vk::Semaphore& operator*();

    static Semaphore create(
        Device& device, vk::SemaphoreCreateInfo create_info
    );

    vk::Semaphore semaphore;
};

struct Surface {
    void destroy(Instance& instance);
    operator vk::SurfaceKHR() const;
    operator VkSurfaceKHR() const;
    bool operator!() const;
    vk::SurfaceKHR* operator->();
    vk::SurfaceKHR& operator*();

    static Surface create(Instance& instance, GLFWwindow* window);

    vk::SurfaceKHR surface;
};

struct Swapchain {
    void recreate(
        PhysicalDevice& physical_device, Device& device, Surface& surface
    );
    void destroy(Device& device);
    operator vk::SwapchainKHR() const;
    operator VkSwapchainKHR() const;
    operator vk::SwapchainKHR&();
    Image next_image(Device& device);
    Image current_image() const;
    ImageView current_image_view() const;
    vk::Semaphore& image_available();
    vk::Semaphore& render_finished();
    vk::Fence& fence();
    bool operator!() const;
    vk::SwapchainKHR* operator->();
    vk::SwapchainKHR& operator*();

    static Swapchain create(
        PhysicalDevice& physical_device,
        Device& device,
        Surface& surface,
        bool vsync = false
    );

    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> images;
    std::vector<ImageView> image_views;
    vk::SurfaceFormatKHR surface_format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
    uint32_t image_index = 0;
    uint32_t current_frame = 0;
    vk::Semaphore image_available_semaphore[2];
    vk::Semaphore render_finished_semaphore[2];
    vk::Fence in_flight_fence[2];
};

struct RenderPass {
    void destroy(Device& device);
    operator vk::RenderPass() const;
    operator VkRenderPass() const;
    bool operator!() const;
    vk::RenderPass* operator->();
    vk::RenderPass& operator*();

    static RenderPass create(
        Device& device, vk::RenderPassCreateInfo create_info
    );

    vk::RenderPass render_pass;
};

struct Framebuffer {
    void destroy(Device& device);
    operator vk::Framebuffer() const;
    operator VkFramebuffer() const;
    bool operator!() const;
    vk::Framebuffer* operator->();
    vk::Framebuffer& operator*();

    static Framebuffer create(
        Device& device, vk::FramebufferCreateInfo create_info
    );

    vk::Framebuffer framebuffer;
};

struct ShaderModule {
    void destroy(Device& device);
    operator vk::ShaderModule() const;
    operator VkShaderModule() const;
    bool operator!() const;
    vk::ShaderModule* operator->();
    vk::ShaderModule& operator*();

    static ShaderModule create(
        Device& device, const vk::ShaderModuleCreateInfo& create_info
    );

    vk::ShaderModule shader_module;
};

struct PipelineCache {
    void destroy(Device& device);
    operator vk::PipelineCache() const;
    operator VkPipelineCache() const;
    bool operator!() const;
    vk::PipelineCache* operator->();
    vk::PipelineCache& operator*();

    static PipelineCache create(Device& device);

    vk::PipelineCache pipeline_cache;
};
}  // namespace components
}  // namespace render_vk
}  // namespace pixel_engine