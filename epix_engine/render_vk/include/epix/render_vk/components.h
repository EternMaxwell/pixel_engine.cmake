#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <unordered_map>
#include <vector>

#include "epix/common.h"
#include "vulkan_headers.h"

namespace epix {
namespace render_vk {
namespace components {
struct RenderContext {};

struct Instance {
    EPIX_API void destroy();
    EPIX_API operator vk::Instance() const;
    EPIX_API operator VkInstance() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Instance* operator->();
    EPIX_API vk::Instance& operator*();

    EPIX_API static Instance create(
        const char* app_name,
        uint32_t app_version,
        std::shared_ptr<spdlog::logger> logger
    );

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debug_messenger;
    std::shared_ptr<spdlog::logger> logger;
};

struct PhysicalDevice {
    EPIX_API operator vk::PhysicalDevice() const;
    EPIX_API operator VkPhysicalDevice() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::PhysicalDevice* operator->();
    EPIX_API vk::PhysicalDevice& operator*();

    EPIX_API static PhysicalDevice create(Instance instance);

    vk::PhysicalDevice physical_device;
};

struct Device {
    EPIX_API void destroy();
    EPIX_API operator vk::Device() const;
    EPIX_API operator VkDevice() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Device* operator->();
    EPIX_API vk::Device& operator*();

    EPIX_API static Device create(
        Instance& instance,
        PhysicalDevice& physical_device,
        vk::QueueFlags queue_flags = vk::QueueFlagBits::eGraphics
    );

    vk::Device device;
    VmaAllocator allocator;
    uint32_t queue_family_index;
};

struct Queue {
    EPIX_API operator vk::Queue() const;
    EPIX_API operator VkQueue() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Queue* operator->();
    EPIX_API vk::Queue& operator*();

    EPIX_API static Queue create(Device& device);

    vk::Queue queue;
};

struct CommandPool {
    EPIX_API void destroy(Device device);
    EPIX_API operator vk::CommandPool() const;
    EPIX_API operator VkCommandPool() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::CommandPool* operator->();
    EPIX_API vk::CommandPool& operator*();

    EPIX_API static CommandPool create(Device& device);

    vk::CommandPool command_pool;
};

struct CommandBuffer {
    EPIX_API void free(Device& device, CommandPool& command_pool);
    EPIX_API operator vk::CommandBuffer() const;
    EPIX_API operator VkCommandBuffer() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::CommandBuffer* operator->();
    EPIX_API vk::CommandBuffer& operator*();

    EPIX_API static CommandBuffer allocate_primary(
        Device& device, CommandPool& command_pool
    );
    EPIX_API static CommandBuffer allocate_secondary(
        Device& device, CommandPool& command_pool
    );
    EPIX_API static std::vector<CommandBuffer> allocate_primary(
        Device& device, CommandPool& command_pool, uint32_t count
    );
    EPIX_API static std::vector<CommandBuffer> allocate_secondary(
        Device& device, CommandPool& command_pool, uint32_t count
    );

    vk::CommandBuffer command_buffer;
};

struct AllocationCreateInfo {
    EPIX_API AllocationCreateInfo();
    EPIX_API AllocationCreateInfo(
        const VmaMemoryUsage& usage, const VmaAllocationCreateFlags& flags
    );

    EPIX_API operator VmaAllocationCreateInfo() const;
    EPIX_API AllocationCreateInfo& setUsage(VmaMemoryUsage usage);
    EPIX_API AllocationCreateInfo& setFlags(VmaAllocationCreateFlags flags);
    EPIX_API AllocationCreateInfo& setRequiredFlags(VkMemoryPropertyFlags flags
    );
    EPIX_API AllocationCreateInfo& setPreferredFlags(VkMemoryPropertyFlags flags
    );
    EPIX_API AllocationCreateInfo& setMemoryTypeBits(uint32_t bits);
    EPIX_API AllocationCreateInfo& setPool(VmaPool pool);
    EPIX_API AllocationCreateInfo& setPUserData(void* data);
    EPIX_API AllocationCreateInfo& setPriority(float priority);
    EPIX_API VmaAllocationCreateInfo& operator*();

    VmaAllocationCreateInfo create_info;
};

struct Buffer {
    EPIX_API void destroy(Device& device);
    EPIX_API void* map(Device& device);
    EPIX_API void unmap(Device& device);
    EPIX_API operator vk::Buffer() const;
    EPIX_API operator VkBuffer() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Buffer* operator->();
    EPIX_API vk::Buffer& operator*();

    EPIX_API static Buffer create(
        Device& device,
        vk::BufferCreateInfo create_info,
        AllocationCreateInfo alloc_info
    );
    EPIX_API static Buffer create_device(
        Device& device, uint64_t size, vk::BufferUsageFlags usage
    );
    EPIX_API static Buffer create_host(
        Device& device, uint64_t size, vk::BufferUsageFlags usage
    );

    vk::Buffer buffer;
    VmaAllocation allocation;
};

struct Image {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::Image() const;
    EPIX_API operator VkImage() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Image* operator->();
    EPIX_API vk::Image& operator*();

    EPIX_API static Image create(
        Device& device,
        vk::ImageCreateInfo create_info,
        AllocationCreateInfo alloc_info
    );
    EPIX_API static Image create_1d(
        Device& device,
        uint32_t width,
        uint32_t levels,
        uint32_t layers,
        vk::Format format,
        vk::ImageUsageFlags usage
    );
    EPIX_API static Image create_2d(
        Device& device,
        uint32_t width,
        uint32_t height,
        uint32_t levels,
        uint32_t layers,
        vk::Format format,
        vk::ImageUsageFlags usage
    );
    EPIX_API static Image create_3d(
        Device& device,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint32_t levels,
        uint32_t layers,
        vk::Format format,
        vk::ImageUsageFlags usage
    );

    vk::Image image;
    VmaAllocation allocation;
};

struct ImageView {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::ImageView() const;
    EPIX_API operator VkImageView() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::ImageView* operator->();
    EPIX_API vk::ImageView& operator*();

    EPIX_API static ImageView create(
        Device& device, vk::ImageViewCreateInfo create_info
    );
    EPIX_API static ImageView create_1d(
        Device& device,
        Image& image,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags,
        uint32_t base_level,
        uint32_t level_count,
        uint32_t base_layer,
        uint32_t layer_count
    );
    EPIX_API static ImageView create_1d(
        Device& device,
        Image& image,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags
    );
    EPIX_API static ImageView create_2d(
        Device& device,
        Image& image,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags,
        uint32_t base_level,
        uint32_t level_count,
        uint32_t base_layer,
        uint32_t layer_count
    );
    EPIX_API static ImageView create_2d(
        Device& device,
        Image& image,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags
    );
    EPIX_API static ImageView create_3d(
        Device& device,
        Image& image,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags,
        uint32_t base_level,
        uint32_t level_count,
        uint32_t base_layer,
        uint32_t layer_count
    );
    EPIX_API static ImageView create_3d(
        Device& device,
        Image& image,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags
    );

    vk::ImageView image_view;
};

struct Sampler {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::Sampler() const;
    EPIX_API operator VkSampler() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Sampler* operator->();
    EPIX_API vk::Sampler& operator*();

    EPIX_API static Sampler create(
        Device& device, vk::SamplerCreateInfo create_info
    );
    EPIX_API static Sampler create(
        Device& device,
        vk::Filter min_filter,
        vk::Filter mag_filter,
        vk::SamplerAddressMode u_address_mode,
        vk::SamplerAddressMode v_address_mode,
        vk::SamplerAddressMode w_address_mode,
        vk::BorderColor border_color = vk::BorderColor::eFloatTransparentBlack
    );

    vk::Sampler sampler;
};

struct DescriptorSetLayout {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::DescriptorSetLayout() const;
    EPIX_API operator VkDescriptorSetLayout() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::DescriptorSetLayout* operator->();
    EPIX_API vk::DescriptorSetLayout& operator*();

    EPIX_API static DescriptorSetLayout create(
        Device& device, vk::DescriptorSetLayoutCreateInfo create_info
    );
    EPIX_API static DescriptorSetLayout create(
        Device& device,
        const std::vector<vk::DescriptorSetLayoutBinding>& bindings
    );

    vk::DescriptorSetLayout descriptor_set_layout;
};

struct PipelineLayout {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::PipelineLayout() const;
    EPIX_API operator VkPipelineLayout() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::PipelineLayout* operator->();
    EPIX_API vk::PipelineLayout& operator*();

    EPIX_API static PipelineLayout create(
        Device& device, vk::PipelineLayoutCreateInfo create_info
    );
    EPIX_API static PipelineLayout create(
        Device& device, DescriptorSetLayout& descriptor_set_layout
    );
    EPIX_API static PipelineLayout create(
        Device& device,
        DescriptorSetLayout& descriptor_set_layout,
        vk::PushConstantRange& push_constant_range
    );
    EPIX_API static PipelineLayout create(
        Device& device,
        std::vector<DescriptorSetLayout>& descriptor_set_layout,
        std::vector<vk::PushConstantRange>& push_constant_ranges
    );

    vk::PipelineLayout pipeline_layout;
};

struct Pipeline {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::Pipeline() const;
    EPIX_API operator VkPipeline() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Pipeline* operator->();
    EPIX_API vk::Pipeline& operator*();

    EPIX_API static Pipeline create(
        Device& device,
        vk::PipelineCache pipeline_cache,
        vk::GraphicsPipelineCreateInfo create_info
    );
    EPIX_API static Pipeline create(
        Device& device, vk::GraphicsPipelineCreateInfo create_info
    );
    EPIX_API static Pipeline create(
        Device& device,
        vk::PipelineCache pipeline_cache,
        vk::ComputePipelineCreateInfo create_info
    );
    EPIX_API static Pipeline create(
        Device& device, vk::ComputePipelineCreateInfo create_info
    );

    vk::Pipeline pipeline;
};

struct DescriptorPool {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::DescriptorPool() const;
    EPIX_API operator VkDescriptorPool() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::DescriptorPool* operator->();
    EPIX_API vk::DescriptorPool& operator*();

    EPIX_API static DescriptorPool create(
        Device& device, vk::DescriptorPoolCreateInfo create_info
    );

    vk::DescriptorPool descriptor_pool;
};

struct DescriptorSet {
    EPIX_API void destroy(Device& device, DescriptorPool& descriptor_pool);
    EPIX_API operator vk::DescriptorSet() const;
    EPIX_API operator VkDescriptorSet() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::DescriptorSet* operator->();
    EPIX_API vk::DescriptorSet& operator*();

    EPIX_API static DescriptorSet create(
        Device& device, vk::DescriptorSetAllocateInfo allocate_info
    );
    EPIX_API static DescriptorSet create(
        Device& device,
        DescriptorPool& descriptor_pool,
        vk::DescriptorSetLayout descriptor_set_layout
    );
    EPIX_API static std::vector<DescriptorSet> create(
        Device& device,
        DescriptorPool& descriptor_pool,
        std::vector<DescriptorSetLayout>& descriptor_set_layouts
    );

    vk::DescriptorSet descriptor_set;
};

struct Fence {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::Fence() const;
    EPIX_API operator VkFence() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Fence* operator->();
    EPIX_API vk::Fence& operator*();

    EPIX_API static Fence create(
        Device& device, vk::FenceCreateInfo create_info
    );

    vk::Fence fence;
};

struct Semaphore {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::Semaphore() const;
    EPIX_API operator VkSemaphore() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Semaphore* operator->();
    EPIX_API vk::Semaphore& operator*();

    EPIX_API static Semaphore create(
        Device& device, vk::SemaphoreCreateInfo create_info
    );

    vk::Semaphore semaphore;
};

struct Surface {
    EPIX_API void destroy(Instance& instance);
    EPIX_API operator vk::SurfaceKHR() const;
    EPIX_API operator VkSurfaceKHR() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::SurfaceKHR* operator->();
    EPIX_API vk::SurfaceKHR& operator*();

    EPIX_API static Surface create(Instance& instance, GLFWwindow* window);

    vk::SurfaceKHR surface;
};

struct Swapchain {
    EPIX_API void recreate(
        PhysicalDevice& physical_device, Device& device, Surface& surface
    );
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::SwapchainKHR() const;
    EPIX_API operator VkSwapchainKHR() const;
    EPIX_API operator vk::SwapchainKHR&();
    EPIX_API Image next_image(Device& device);
    EPIX_API Image current_image() const;
    EPIX_API ImageView current_image_view() const;
    EPIX_API vk::Fence& fence();
    EPIX_API bool operator!() const;
    EPIX_API vk::SwapchainKHR* operator->();
    EPIX_API vk::SwapchainKHR& operator*();

    EPIX_API static Swapchain create(
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
    uint32_t image_index   = 0;
    uint32_t current_frame = 0;
    vk::Fence in_flight_fence[2];
};

struct RenderPass {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::RenderPass() const;
    EPIX_API operator VkRenderPass() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::RenderPass* operator->();
    EPIX_API vk::RenderPass& operator*();

    EPIX_API static RenderPass create(
        Device& device, vk::RenderPassCreateInfo create_info
    );

    vk::RenderPass render_pass;
};

struct Framebuffer {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::Framebuffer() const;
    EPIX_API operator VkFramebuffer() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::Framebuffer* operator->();
    EPIX_API vk::Framebuffer& operator*();

    EPIX_API static Framebuffer create(
        Device& device, vk::FramebufferCreateInfo create_info
    );

    vk::Framebuffer framebuffer;
};

struct ShaderModule {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::ShaderModule() const;
    EPIX_API operator VkShaderModule() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::ShaderModule* operator->();
    EPIX_API vk::ShaderModule& operator*();

    EPIX_API static ShaderModule create(
        Device& device, const vk::ShaderModuleCreateInfo& create_info
    );

    vk::ShaderModule shader_module;
};

struct PipelineCache {
    EPIX_API void destroy(Device& device);
    EPIX_API operator vk::PipelineCache() const;
    EPIX_API operator VkPipelineCache() const;
    EPIX_API bool operator!() const;
    EPIX_API vk::PipelineCache* operator->();
    EPIX_API vk::PipelineCache& operator*();

    EPIX_API static PipelineCache create(Device& device);

    vk::PipelineCache pipeline_cache;
};

struct RenderTarget {
    std::vector<std::tuple<Image, ImageView, vk::Format>> color_attachments;
    std::tuple<Image, ImageView, vk::Format> depth_attachment;
    vk::Extent2D extent;

    EPIX_API RenderTarget& set_extent(uint32_t width, uint32_t height);
    EPIX_API RenderTarget& add_color_image(
        Image& color_image, vk::Format color_format
    );
    EPIX_API RenderTarget& add_color_image(
        Device& device, Image& color_image, vk::Format color_format
    );
    EPIX_API RenderTarget& add_color_image(
        Image& color_image, ImageView& color_image_view, vk::Format color_format
    );
    EPIX_API RenderTarget& set_depth_image(
        Image& depth_image, vk::Format depth_format
    );
    EPIX_API RenderTarget& set_depth_image(
        Device& device, Image& depth_image, vk::Format depth_format
    );
    EPIX_API RenderTarget& set_depth_image(
        Image& depth_image, ImageView& depth_image_view, vk::Format depth_format
    );
    EPIX_API RenderTarget& complete(Device& device);
};
}  // namespace components
}  // namespace render_vk
}  // namespace epix